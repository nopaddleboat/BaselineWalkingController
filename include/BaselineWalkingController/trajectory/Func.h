#pragma once

#include <array>
#include <cmath>
#include <functional>
#include <limits>
#include <map>
#include <memory>
#include <vector>

#include <mc_rtc/logging.h>

namespace BWC
{
/** \brief Mathematical function.
    \tparam T function value type
*/
template<class T>
class Func
{
public:
  /** \brief Constructor. */
  Func() {}

  /** \brief Evaluate function value.
      \param t arugment of function
  */
  virtual T operator()(double t) const = 0;

  /** \brief Evaluate function derivative value.
      \param t arugment of function
      \param order derivative order
  */
  virtual T derivative(double t, int order = 1) const = 0;

  /** \brief Get lower limit of domain. */
  virtual double domainLowerLimit() const
  {
    return std::numeric_limits<double>::lowest();
  }

  /** \brief Get upper limit of domain. */
  virtual double domainUpperLimit() const
  {
    return std::numeric_limits<double>::max();
  }
};

/** \brief Piecewise function.
    \tparam T function value type
*/
template<class T>
class PiecewiseFunc : public Func<T>
{
public:
  /** \brief Constructor. */
  PiecewiseFunc() {}

  /** \brief Evaluate function value.
      \param t arugment of function
  */
  virtual T operator()(double t) const override
  {
    checkArg(t);
    auto funcIt = funcs_.lower_bound(t);
    return (*(funcIt->second))(t);
  }

  /** \brief Evaluate function derivative value.
      \param t arugment of function
      \param order derivative order
  */
  virtual T derivative(double t, int order = 1) const override
  {
    checkArg(t);
    auto funcIt = funcs_.lower_bound(t);
    return (funcIt->second)->derivative(t, order);
  }

  /** \brief Get lower limit of domain. */
  virtual double domainLowerLimit() const override
  {
    return tLowerLimit_;
  }

  /** \brief Get upper limit of domain. */
  virtual double domainUpperLimit() const override
  {
    if(funcs_.empty())
    {
      return std::numeric_limits<double>::max();
    }
    return funcs_.rbegin()->first;
  }

  /** \brief Clear function. */
  void clearFuncs()
  {
    funcs_.clear();
    tLowerLimit_ = std::numeric_limits<double>::lowest();
  }

  /** \brief Add function.
      \param t upper bound of domain
      \param func function
  */
  void appendFunc(double t, std::shared_ptr<Func<T>> func)
  {
    funcs_.insert(std::pair<double, std::shared_ptr<Func<T>>>(t, func));
  }

  /** \brief Set lower limit of domain
      \param t lower limit of domain
  */
  void setDomainLowerLimit(double t)
  {
    tLowerLimit_ = t;
  }

protected:
  /** \brief Check argument of function.
      \param t arugment of function
  */
  void checkArg(double t) const
  {
    if(t < tLowerLimit_ || funcs_.rbegin()->first < t)
    {
      mc_rtc::log::error_and_throw("[PiecewiseFunc] Argument is out of function range. it should be {} <= {} <= {}",
                                   tLowerLimit_, t, funcs_.rbegin()->first);
    }
  }

protected:
  //! Map of upper bound of domain and function
  std::map<double, std::shared_ptr<Func<T>>> funcs_;

  //! Lower limit of domain of this function
  double tLowerLimit_ = std::numeric_limits<double>::lowest();
};

/** \brief Polynomial function.
    \tparam T function value type
    \tparam Order order of polynomial function
*/
template<class T, int Order>
class Polynomial : public Func<T>
{
public:
  /** \brief Constructor.
      \param coeff coefficients of polynomial (from low order (i.e., constant term) to high order)
      \param t0 offset of function arugment
  */
  Polynomial(const std::array<T, Order + 1> & coeff, double t0 = 0.0) : coeff_(coeff), t0_(t0) {}

  /** \brief Get polynomial order. */
  int order() const
  {
    return Order;
  }

  /** \brief Evaluate function value.
      \param t arugment of function
  */
  virtual T operator()(double t) const override
  {
    T ret = coeff_[0];
    for(int i = 0; i < Order; i++)
    {
      ret += coeff_[i + 1] * std::pow(t - t0_, i + 1);
    }
    return ret;
  }

  /** \brief Evaluate function derivative value.
      \param t arugment of function
      \param order derivative order
  */
  virtual T derivative(double t, int derivative_order = 1) const override
  {
    if(derivative_order > Order)
    {
      // \todo does not work for Eigen::VectorXd
      T ret;
      ret.setZero();
      return ret;
    }

    T ret = coeff_[derivative_order];
    for(int j = 0; j < derivative_order - 1; j++)
    {
      ret *= (derivative_order - j);
    }

    for(int i = 0; i < Order - derivative_order; i++)
    {
      T term = coeff_[i + 1 + derivative_order] * std::pow(t - t0_, i + 1);
      for(int j = 0; j < derivative_order; j++)
      {
        term *= (i + 1 + derivative_order - j);
      }
      ret += term;
    }

    return ret;
  }

protected:
  //! Coefficients from low order (i.e., constant term) to high order
  std::array<T, Order + 1> coeff_;

  //! Offset of function arugment
  double t0_;
};

/** \brief Constant function.
    \tparam T function value type
*/
template<class T>
class Constant : public Polynomial<T, 0>
{
public:
  /** \brief Constructor.
      \param value constant value
  */
  Constant(const T & value) : Polynomial<T, 0>(std::array<T, 1>{value}) {}

  /** \brief Evaluate function value.
      \param t arugment of function (which does not affect the return value)
  */
  virtual T operator()(double t = 0.0) const override
  {
    return Polynomial<T, 0>::operator()(t);
  }
};

/** \brief Linear polynomial function. */
template<class T>
using LinearPolynomial = Polynomial<T, 1>;

/** \brief Quadratic polynomial function. */
template<class T>
using QuadraticPolynomial = Polynomial<T, 2>;

/** \brief Cubic polynomial function. */
template<class T>
using CubicPolynomial = Polynomial<T, 3>;
} // namespace BWC
