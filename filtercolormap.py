from math import floor
import cv2
import numpy as np
import sys


grad = cv2.imread("c.png")

for Y in range(1, 128):
    Gk = cv2.getGaussianKernel(Y * 4 + 1, -1)
    print (Y)
    if (Y < 5): print (Gk)
    for X in range(128):
        B, G, R, N = 0, 0, 0, 0
        for i in range(len(Gk)):
            nx = X + i - floor(len(Gk) / 2)
            if nx >= 0 and nx < 128:
                N += Gk[i]
                B += grad[0, nx, 0] * Gk[i]
                G += grad[0, nx, 1] * Gk[i]
                R += grad[0, nx, 2] * Gk[i]
        if (N > 0):
            B /= N
            G /= N
            R /= N
        grad[Y, X, 0] = B
        grad[Y, X, 1] = G
        grad[Y, X, 2] = R
        #print(f"{R}, {G}, {B}, {N}")

cv2.imwrite("c.png", grad)