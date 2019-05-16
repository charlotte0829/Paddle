#include "paddle/fluid/framework/eigen.h"
#include "paddle/fluid/framework/operator.h"
#include "paddle/fluid/lite/core/kernel.h"
#include "paddle/fluid/lite/core/op_registry.h"
#include "paddle/fluid/operators/activation_op.h"

namespace paddle {
namespace lite {
namespace kernels {
namespace x86 {

template <typename Functor>
void Activate(const platform::CPUDeviceContext& context,
              const framework::LoDTensor* X, framework::LoDTensor* Out) {
  using T = typename Functor::ELEMENT_TYPE;
  auto* place = context.eigen_device();
  auto x =
      framework::EigenVector<T>::Flatten(paddle::operators::detail::Ref(X));
  auto out =
      framework::EigenVector<T>::Flatten(paddle::operators::detail::Ref(Out));
  Functor()(*place, x, out);
}

template <typename Functor>
void ActivateGrad(const platform::CPUDeviceContext& context,
                  const framework::LoDTensor* X,
                  const framework::LoDTensor* Out,
                  const framework::LoDTensor* Out_grad,
                  framework::LoDTensor* X_grad) {
  using T = typename Functor::ELEMENT_TYPE;
  auto* place = context.eigen_device();
  auto x =
      framework::EigenVector<T>::Flatten(paddle::operators::detail::Ref(X));
  auto out =
      framework::EigenVector<T>::Flatten(paddle::operators::detail::Ref(Out));
  auto x_grad = framework::EigenVector<T>::Flatten(
      paddle::operators::detail::Ref(X_grad));
  auto out_grad = framework::EigenVector<T>::Flatten(
      paddle::operators::detail::Ref(Out_grad));
  Functor()(*place, x, out, out_grad, x_grad);
}

template <typename T>
class SquareCompute : public KernelLite<TARGET(kHost), PRECISION(kFloat)> {
 public:
  using param_t = operators::ActivationParam;

  void Run() override {
    auto& context = context_->As<X86Context>();
    auto& param = *param_.get_mutable<operators::ActivationParam>();
    CHECK(context.x86_device_context);

    param.Out->template mutable_data<T>();
    Activate<paddle::operators::SquareFunctor<T>>(*context.x86_device_context,
                                                  &param.X->raw_tensor(),
                                                  &param.Out->raw_tensor());
  }

  // TargetType target() const override;
  // PrecisionType precision() const override;

  virtual ~SquareCompute() = default;
};

template <typename T>
class SquareGradCompute : public KernelLite<TARGET(kHost), PRECISION(kFloat)> {
 public:
  using param_t = operators::ActivationGradParam;

  void Run() override {
    auto& context = context_->As<X86Context>();
    auto& param = *param_.get_mutable<operators::ActivationGradParam>();
    CHECK(context.x86_device_context);
    param.X_grad->template mutable_data<T>();

    ActivateGrad<paddle::operators::SquareGradFunctor<T>>(
        *context.x86_device_context, &param.X->raw_tensor(),
        &param.Out->raw_tensor(), &param.Out_grad->raw_tensor(),
        &param.X_grad->raw_tensor());
  }

  // TargetType target() const override;
  // PrecisionType precision() const override;

  virtual ~SquareGradCompute() = default;
};

}  // namespace x86
}  // namespace kernels
}  // namespace lite
}  // namespace paddle

// float
REGISTER_LITE_KERNEL(square, kX86, kFloat, kNCHW,
                     paddle::lite::kernels::x86::SquareCompute<float>, def)
    .BindInput("Input", {LiteType::GetTensorTy(TARGET(kHost))})
    .BindInput("Bias", {LiteType::GetTensorTy(TARGET(kHost))})
    .BindInput("W", {LiteType::GetTensorTy(TARGET(kHost))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kHost))})
    .Finalize();

REGISTER_LITE_KERNEL(square_grad, kX86, kFloat, kNCHW,
                     paddle::lite::kernels::x86::SquareGradCompute<float>, def)
    .BindInput("Input", {LiteType::GetTensorTy(TARGET(kHost))})
    .BindInput("Bias", {LiteType::GetTensorTy(TARGET(kHost))})
    .BindInput("W", {LiteType::GetTensorTy(TARGET(kHost))})
    .BindOutput("Out", {LiteType::GetTensorTy(TARGET(kHost))})
    .Finalize();