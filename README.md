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
- git clone https://github.com/your-username/Gshare_Perceptron_Predictor.git
- cd Gshare_Perceptron_Predictor/src
- make clean
- make
- ./predictor_app ../traces/<input_file_name> --verbose (for running all predictors)
- ./predictor_app ../traces/<input_file_name> --<options> --verbose
- options = --gshare, --perceptron, --custom, --tournament, --2bit

##Configurable Parameters & Tuning
#Gshare
- Global history length (number of past branch outcomes to maintain)
- Size of the pattern history table (PHT)

#Perceptron
- Number of perceptrons / table size
- History length 
- Weight representation
- Threshold for perceptron output to decide “taken” vs “not taken”

#Hybrid selection mechanism
- The “chooser” table logic that selects which predictor to trust per branch based on indexing of gshare

##The above preceptron accuracy is compared with Tounament and 2-Bit predictor for performance evaluation. The predictors are implemented in the same system.

##Flow diagram

graph TB
    Start([Start]) --> Parse[Parse Arguments<br/>& Open Trace]
    Parse --> Init[Initialize Predictor]
    
    Init --> B2{Predictor<br/>Type}
    
    B2 -->|2-Bit| Init1["<b>2-Bit Counter Table</b><br/>Entries: 2^indexBits<br/>Size: 2 bits/entry"]
    B2 -->|GShare| Init2["<b>GShare</b><br/>BHT: 2^ghistoryBits<br/>GHR: ghistoryBits<br/>Size: 2 bits/entry"]
    B2 -->|Perceptron| Init3["<b>Perceptron</b><br/>Entries: 2^indexBits<br/>Weights: historyLen+1<br/>Size: 8 bits/weight"]
    B2 -->|Tournament| Init4["<b>Tournament</b><br/>Global BHT<br/>Local PHT & BHT<br/>Chooser Table"]
    B2 -->|Custom| Init5["<b>Custom Hybrid</b><br/>GShare Component<br/>Perceptron Component<br/>Chooser Table"]
    
    Init1 --> Loop[/<b>For Each Branch</b><br/>Read PC & Outcome/]
    Init2 --> Loop
    Init3 --> Loop
    Init4 --> Loop
    Init5 --> Loop
    
    Loop --> Pred["<b>Make Prediction</b><br/>prediction = make_prediction(PC)"]
    
    Pred --> Compare{"Prediction<br/>==<br/>Outcome?"}
    
    Compare -->|No| Miss["<b>Misprediction</b><br/>mispred_count++"]
    Compare -->|Yes| Train
    
    Miss --> Train["<b>Train Predictor</b><br/>train_predictor(PC, outcome)"]
    
    Train --> Update{Update<br/>Method}
    
    Update -->|2-Bit| U1["Increment/Decrement<br/>2-bit saturating counter"]
    Update -->|GShare| U2["Update BHT counter<br/>Shift outcome into GHR"]
    Update -->|Perceptron| U3["Adjust weights if:<br/>• Misprediction, or<br/>• |y| ≤ θ<br/>Update GHR"]
    Update -->|Tournament| U4["Update Global predictor<br/>Update Local predictor<br/>Update Chooser"]
    Update -->|Custom| U5["Train GShare<br/>Train Perceptron<br/>Update Chooser"]
    
    U1 --> Check{More<br/>Branches?}
    U2 --> Check
    U3 --> Check
    U4 --> Check
    U5 --> Check
    
    Check -->|Yes| Loop
    Check -->|No| Stats["<b>Calculate Statistics</b><br/>• Accuracy = (1 - mispred/total) × 100%<br/>• MPKI = (mispred/total) × 1000<br/>• Hardware Cost (bits)<br/>• Chooser Usage (Custom only)"]
    
    Stats --> Output["<b>Output Results</b><br/>Print to console"]
    Output --> End([End])
    
    classDef initStyle fill:#E3F2FD,stroke:#1976D2,stroke-width:3px,color:#000
    classDef predStyle fill:#FFF3E0,stroke:#F57C00,stroke-width:3px,color:#000
    classDef trainStyle fill:#F3E5F5,stroke:#7B1FA2,stroke-width:3px,color:#000
    classDef statsStyle fill:#E8F5E9,stroke:#388E3C,stroke-width:3px,color:#000
    classDef decisionStyle fill:#FFF9C4,stroke:#F9A825,stroke-width:3px,color:#000
    
    class Init1,Init2,Init3,Init4,Init5,Init initStyle
    class Pred,Loop predStyle
    class Train,U1,U2,U3,U4,U5,Miss trainStyle
    class Stats,Output statsStyle
    class B2,Compare,Update,Check decisionStyle
