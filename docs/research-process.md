# 建模与核心算法

## 1. 示例电路与通用方法

5F6-A 在这里是完整的实现案例。DK Method 本身不依赖某个音箱型号：为新电路重新定义支路、节点、储能元件和非线性端口后，后续的状态更新与 Newton 求解流程保持不变。

```text
Input → 第一前级近似 → Volume/Bright IIR
      → 两只 12AX7 + Treble/Bass/Middle DK 网络
      → 4× oversampling → Output
```

![当前 C++ 实现的 Nodal DK 电路](images/dk-circuit-topology.svg)

线性 RC 关系可用 [Online Circuit Solver](https://onlinecircuitsolver.com/) 辅助计算；[LiveSPICE](https://www.livespice.org/) 用于参考和比较实时电路仿真。完整理论来源见[参考资料](references.md)。

## 2. DK Method

DK Method 将电路拆成线性网络和非线性端口。电阻、电容和电压源只需在采样率或元件值改变时生成矩阵；每个采样点只求电子管端口构成的小型非线性系统。

令 `N_r`、`N_x`、`N_u`、`N_o`、`N_n` 分别为电阻、电容、输入、输出和非线性端口的关联矩阵。电阻电导和梯形积分后的电容伴随电导为

$$
G_r=\mathrm{diag}\left(\frac{1}{R_i}\right),
\qquad
G_x=\mathrm{diag}\left(\frac{2C_i}{T}\right),
$$

其中 `T=1/f_s`。加入理想电压源约束后，扩展节点矩阵为

$$
S=
\begin{bmatrix}
N_r^T G_r N_r+N_x^T G_x N_x & N_u^T\\
N_u & 0
\end{bmatrix}.
$$

把关联矩阵补到与 `S` 相同的列数后，记为 `N_xp`、`N_op`、`N_np`，输入选择矩阵记为 `N_up`。离散系统写成

$$
\begin{aligned}
x_{n+1}&=Ax_n+Bu_n+Ci_n,\\
y_n&=Dx_n+Eu_n+Fi_n,\\
v_n&=Gx_n+Hu_n+Ki_n.
\end{aligned}
$$

代码中的九个矩阵由 `S^{-1}` 直接构造：

$$
\begin{aligned}
A&=2G_xN_{xp}S^{-1}N_{xp}^T-I,&
B&=2G_xN_{xp}S^{-1}N_{up},\\
C&=2G_xN_{xp}S^{-1}N_{np}^T,&
D&=N_{op}S^{-1}N_{xp}^T,\\
E&=N_{op}S^{-1}N_{up},&
F&=N_{op}S^{-1}N_{np}^T,\\
G&=N_{np}S^{-1}N_{xp}^T,&
H&=N_{np}S^{-1}N_{up},\\
K&=N_{np}S^{-1}N_{np}^T.
\end{aligned}
$$

`x` 是三个电容的状态，`u` 是音频输入和 325 V 电源，`v/i` 是两只三极管的四组非线性端口电压与电流。

## 3. 12AX7 非线性方程

为了让电流曲线连续且可求导，使用 softplus 函数

$$
L_C(q)=\frac{\log(1+e^{Cq})}{C}.
$$

栅极电流、阴极总电流和板极端口电流为

$$
\begin{aligned}
I_g&=-G_gL_{C_g}(V_{gk})^\xi,\\
I_k&=-G_pL_{C_p}\left(\frac{V_{pk}}{\mu}+V_{gk}\right)^\gamma,\\
I_p&=I_k-I_g.
\end{aligned}
$$

当前参数为：`G_g=606 µS`、`ξ=1.354`、`C_g=13.9`、`G_p=2.14 mS`、`γ=1.303`、`C_p=3.04`、`μ=100.8`。

softplus 的导数是 logistic 函数：

$$
\frac{dL_C(q)}{dq}=\frac{1}{1+e^{-Cq}}.
$$

因此可以解析得到每只三极管的 2×2 电流雅可比

$$
J_i=
\begin{bmatrix}
\partial I_g/\partial V_{gk} & 0\\
\partial I_p/\partial V_{gk} & \partial I_p/\partial V_{pk}
\end{bmatrix}.
$$

解析雅可比避免在每个采样点用有限差分重复计算电流；测试中再用中心有限差分验证它。

## 4. Newton 与伪逆

对当前状态和输入先计算

$$
P=Gx_n+Hu_n.
$$

非线性端口必须满足

$$
r(v)=P+Ki(v)-v=0,
\qquad
J_r(v)=KJ_i(v)-I.
$$

Newton 迭代先求步长，再更新端口电压：

$$
J_r\Delta v=r,
\qquad
v_{k+1}=v_k-\lambda\Delta v.
$$

其中阻尼 `λ` 从 1 开始；残差未下降时不断减半。

若使用 SVD 伪逆，先分解

$$
J_r=U\Sigma V^T,
$$

再对大于阈值的奇异值取倒数：

$$
J_r^+=V\Sigma^+U^T,
\qquad
\Delta v=J_r^+r.
$$

$$
\left(\Sigma^+\right)_{ii}=
\begin{cases}
1/\sigma_i,&\sigma_i>\tau,\\
0,&\sigma_i\le\tau,
\end{cases}
\qquad
\tau=\epsilon\,\max(m,n)\,\sigma_{\max}.
$$

伪逆能为奇异、病态或非方阵系统给出最小二乘步长，但逐采样 SVD 成本较高。当前系统固定为通常可逆的 4×4 方阵，因此代码使用带部分主元的高斯消元直接解 `J_r Δv=r`；矩阵接近奇异时停止本次迭代，而不是在实时路径计算 SVD。

## 5. 代码对应

- `NodalDKModel::buildMatrices()`：构造 `S` 和 `A…K`。
- `TriodeModel::evaluate()`：计算 `i(v)` 和 `J_i(v)`。
- `NodalDKModel::solveNonlinear()`：残差、Newton、阻尼线搜索。
- `math::solve()`（`LinearAlgebra.h`）：带部分主元的高斯消元。

当前模型不包含功率放大级、输出变压器、扬声器和箱体，也尚未完成实机测量校准。
