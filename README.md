# Gshare_Perceptron_Predictor

## Overview  
This project implements a hybrid branch predictor that combines a Gshare branch predictor and a Perceptron branch predictor. 
The goal is to leverage the complementary strengths of both prediction strategies — Gshare’s simple, fast global-history based prediction, 
and Perceptron’s ability to learn complex patterns using a linear classifier — to achieve improved overall branch prediction accuracy.  


## Project Structure  
├─ src/ # source code for predictors and simulation
├─ traces/ # sample branch traces or benchmark traces for testing
├─ README.md
├─ Makefile # scripts for building and running simulations

## Features  

- Implementation of Gshare predictor (global history + PC-based indexing)  
- Implementation of Perceptron predictor (history-based linear classifier)  
- Hybrid predictor that chooses between Gshare and Perceptron using chooser table indexed by Gshare  
- Support for configurable parameters, e.g.:  
  - Global history length / size for Gshare  
  - Number of perceptrons, history length, threshold values for Perceptron predictor  
- Evaluation framework: ability to feed branch traces, simulate prediction, and compute misprediction rates / accuracy  

## How to Build & Run  
git clone https://github.com/your-username/Gshare_Perceptron_Predictor.git
cd Gshare_Perceptron_Predictor/src
make clean
make
