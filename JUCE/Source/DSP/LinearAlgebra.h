// SPDX-License-Identifier: AGPL-3.0-or-later
#pragma once

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <limits>

namespace bassman::math
{
template <std::size_t Rows, std::size_t Cols>
using Matrix = std::array<std::array<double, Cols>, Rows>;

template <std::size_t Size>
using Vector = std::array<double, Size>;

template <std::size_t Rows, std::size_t Cols>
constexpr Matrix<Rows, Cols> zeros() noexcept
{
    return {};
}

template <std::size_t Size>
constexpr Matrix<Size, Size> identity() noexcept
{
    Matrix<Size, Size> result {};
    for (std::size_t i = 0; i < Size; ++i)
        result[i][i] = 1.0;
    return result;
}

template <std::size_t Rows, std::size_t Inner, std::size_t Cols>
Matrix<Rows, Cols> multiply(const Matrix<Rows, Inner>& left,
                            const Matrix<Inner, Cols>& right) noexcept
{
    Matrix<Rows, Cols> result {};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t inner = 0; inner < Inner; ++inner)
            for (std::size_t col = 0; col < Cols; ++col)
                result[row][col] += left[row][inner] * right[inner][col];
    return result;
}

template <std::size_t Rows, std::size_t Cols>
Vector<Rows> multiply(const Matrix<Rows, Cols>& matrix,
                      const Vector<Cols>& vector) noexcept
{
    Vector<Rows> result {};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t col = 0; col < Cols; ++col)
            result[row] += matrix[row][col] * vector[col];
    return result;
}

template <std::size_t Rows, std::size_t Cols>
Matrix<Cols, Rows> transpose(const Matrix<Rows, Cols>& input) noexcept
{
    Matrix<Cols, Rows> result {};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t col = 0; col < Cols; ++col)
            result[col][row] = input[row][col];
    return result;
}

template <std::size_t Rows, std::size_t Cols>
Matrix<Rows, Cols> add(const Matrix<Rows, Cols>& left,
                       const Matrix<Rows, Cols>& right) noexcept
{
    Matrix<Rows, Cols> result {};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t col = 0; col < Cols; ++col)
            result[row][col] = left[row][col] + right[row][col];
    return result;
}

template <std::size_t Rows, std::size_t Cols>
Matrix<Rows, Cols> subtract(const Matrix<Rows, Cols>& left,
                            const Matrix<Rows, Cols>& right) noexcept
{
    Matrix<Rows, Cols> result {};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t col = 0; col < Cols; ++col)
            result[row][col] = left[row][col] - right[row][col];
    return result;
}

template <std::size_t Rows, std::size_t Cols>
Matrix<Rows, Cols> scale(const Matrix<Rows, Cols>& input, double factor) noexcept
{
    Matrix<Rows, Cols> result {};
    for (std::size_t row = 0; row < Rows; ++row)
        for (std::size_t col = 0; col < Cols; ++col)
            result[row][col] = input[row][col] * factor;
    return result;
}

template <std::size_t Size>
bool solve(Matrix<Size, Size> matrix, Vector<Size> rhs, Vector<Size>& result) noexcept
{
    constexpr double pivotTolerance = 64.0 * std::numeric_limits<double>::epsilon();

    for (std::size_t pivot = 0; pivot < Size; ++pivot)
    {
        std::size_t bestRow = pivot;
        double bestMagnitude = std::abs(matrix[pivot][pivot]);
        for (std::size_t row = pivot + 1; row < Size; ++row)
        {
            const auto magnitude = std::abs(matrix[row][pivot]);
            if (magnitude > bestMagnitude)
            {
                bestMagnitude = magnitude;
                bestRow = row;
            }
        }

        if (!std::isfinite(bestMagnitude) || bestMagnitude <= pivotTolerance)
            return false;

        if (bestRow != pivot)
        {
            std::swap(matrix[pivot], matrix[bestRow]);
            std::swap(rhs[pivot], rhs[bestRow]);
        }

        for (std::size_t row = pivot + 1; row < Size; ++row)
        {
            const auto factor = matrix[row][pivot] / matrix[pivot][pivot];
            matrix[row][pivot] = 0.0;
            for (std::size_t col = pivot + 1; col < Size; ++col)
                matrix[row][col] -= factor * matrix[pivot][col];
            rhs[row] -= factor * rhs[pivot];
        }
    }

    for (std::size_t offset = 0; offset < Size; ++offset)
    {
        const auto row = Size - 1 - offset;
        double value = rhs[row];
        for (std::size_t col = row + 1; col < Size; ++col)
            value -= matrix[row][col] * result[col];
        result[row] = value / matrix[row][row];
        if (!std::isfinite(result[row]))
            return false;
    }
    return true;
}

// Fixed-size, allocation-free form of the column-pivoted Householder QR method
// used in https://github.com/chaodede/solving-least-squares-with-QR.
// The current DK Newton system is square; QR still provides a stable
// least-squares solve without explicitly forming a Jacobian inverse.
template <std::size_t Size>
bool solveLeastSquaresQr(Matrix<Size, Size> matrix,
                         Vector<Size> rhs,
                         Vector<Size>& result) noexcept
{
    std::array<std::size_t, Size> permutation {};
    double matrixScale = 0.0;
    for (std::size_t col = 0; col < Size; ++col)
    {
        permutation[col] = col;
        double normSquared = 0.0;
        for (std::size_t row = 0; row < Size; ++row)
            normSquared += matrix[row][col] * matrix[row][col];
        matrixScale = std::max(matrixScale, std::sqrt(normSquared));
    }

    if (!std::isfinite(matrixScale) || matrixScale == 0.0)
        return false;

    const auto rankTolerance = std::numeric_limits<double>::epsilon()
        * static_cast<double>(Size) * matrixScale;

    for (std::size_t pivot = 0; pivot < Size; ++pivot)
    {
        std::size_t bestColumn = pivot;
        double bestNormSquared = -1.0;
        for (std::size_t col = pivot; col < Size; ++col)
        {
            double normSquared = 0.0;
            for (std::size_t row = pivot; row < Size; ++row)
                normSquared += matrix[row][col] * matrix[row][col];
            if (normSquared > bestNormSquared)
            {
                bestNormSquared = normSquared;
                bestColumn = col;
            }
        }

        if (!std::isfinite(bestNormSquared)
            || std::sqrt(std::max(0.0, bestNormSquared)) <= rankTolerance)
            return false;

        if (bestColumn != pivot)
        {
            for (std::size_t row = 0; row < Size; ++row)
                std::swap(matrix[row][pivot], matrix[row][bestColumn]);
            std::swap(permutation[pivot], permutation[bestColumn]);
        }

        double columnNormSquared = 0.0;
        for (std::size_t row = pivot; row < Size; ++row)
            columnNormSquared += matrix[row][pivot] * matrix[row][pivot];
        const auto columnNorm = std::sqrt(columnNormSquared);
        const auto diagonal = -std::copysign(columnNorm, matrix[pivot][pivot]);

        Vector<Size> householder {};
        for (std::size_t row = pivot; row < Size; ++row)
            householder[row] = matrix[row][pivot];
        householder[pivot] -= diagonal;

        double householderNormSquared = 0.0;
        for (std::size_t row = pivot; row < Size; ++row)
            householderNormSquared += householder[row] * householder[row];
        if (!std::isfinite(householderNormSquared) || householderNormSquared == 0.0)
            return false;
        const auto reflectionScale = 2.0 / householderNormSquared;

        for (std::size_t col = pivot; col < Size; ++col)
        {
            double projection = 0.0;
            for (std::size_t row = pivot; row < Size; ++row)
                projection += householder[row] * matrix[row][col];
            projection *= reflectionScale;
            for (std::size_t row = pivot; row < Size; ++row)
                matrix[row][col] -= householder[row] * projection;
        }

        double rhsProjection = 0.0;
        for (std::size_t row = pivot; row < Size; ++row)
            rhsProjection += householder[row] * rhs[row];
        rhsProjection *= reflectionScale;
        for (std::size_t row = pivot; row < Size; ++row)
            rhs[row] -= householder[row] * rhsProjection;

        matrix[pivot][pivot] = diagonal;
        for (std::size_t row = pivot + 1; row < Size; ++row)
            matrix[row][pivot] = 0.0;
    }

    Vector<Size> permutedSolution {};
    for (std::size_t offset = 0; offset < Size; ++offset)
    {
        const auto row = Size - 1 - offset;
        double value = rhs[row];
        for (std::size_t col = row + 1; col < Size; ++col)
            value -= matrix[row][col] * permutedSolution[col];
        if (std::abs(matrix[row][row]) <= rankTolerance)
            return false;
        permutedSolution[row] = value / matrix[row][row];
        if (!std::isfinite(permutedSolution[row]))
            return false;
    }

    result = {};
    for (std::size_t col = 0; col < Size; ++col)
        result[permutation[col]] = permutedSolution[col];
    return true;
}

template <std::size_t Size>
bool invert(const Matrix<Size, Size>& input, Matrix<Size, Size>& output) noexcept
{
    Matrix<Size, Size> inverse {};
    for (std::size_t col = 0; col < Size; ++col)
    {
        Vector<Size> basis {};
        Vector<Size> solution {};
        basis[col] = 1.0;
        if (!solve(input, basis, solution))
            return false;
        for (std::size_t row = 0; row < Size; ++row)
            inverse[row][col] = solution[row];
    }
    output = inverse;
    return true;
}

template <std::size_t Size>
double maxAbs(const Vector<Size>& vector) noexcept
{
    double maximum = 0.0;
    for (const auto value : vector)
        maximum = std::max(maximum, std::abs(value));
    return maximum;
}
} // namespace bassman::math
