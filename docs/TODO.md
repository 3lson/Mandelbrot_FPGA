# Project Roadmap & Future Work

This document outlines the planned future work an architectural refinements for the Mandelbrot Set Accelerator project. 

## Feature Enhancements

* Additional Color Palettes: Implement several new color mapping schemes in the `color_mapper` module to provide more aesthetic choices for the user.
* 64-bit Fixed-Point Precision: Implement a 64-bit precision data path as a configuration option. This would involve significant Verilog modification but would allow for much deeper zooms into the fractal before hitting precision limits.

## Code Quality and DevOps

CI: Set Up GitHub actions workflow for running a verilog linter and execute the simulation testbenches