from setuptools import setup
from torch.utils.cpp_extension import BuildExtension, CppExtension
import torch
import os

torch_path = os.path.dirname(torch.__file__)

setup(
    name="interpolation",
    version='1.0',
    ext_modules=[
        CppExtension(
            'interpolation',
            sources=['interpolation.cpp'],
            include_dirs=[
                os.path.join(torch_path, 'include'),
                os.path.join(torch_path, 'include', 'torch', 'csrc', 'api', 'include')
            ],
            library_dirs=[os.path.join(torch_path, 'lib')],
            libraries=['torch', 'torch_python']
        ),
    ],
    cmdclass={
        'build_ext': BuildExtension
    }
)