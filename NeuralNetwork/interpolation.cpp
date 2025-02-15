#include <torch/extension.h>


torch::Tensor trilinear_interpolation(
    torch::Tensor fests,
    torch::Tensor pointer
) {
    return fests;
}

PYBIND11_MODULE(TORCH_EXTENSION_NAME , m) {
    m.def("trilinear_interpolation", &trilinear_interpolation, "Trilinear interpolation (CUDA)");
}
