import torch
import interpolation

feats = torch.ones(2)
point = torch.zeros(2)

out = interpolation.trilinear_interpolation(feats, point)
print(out)