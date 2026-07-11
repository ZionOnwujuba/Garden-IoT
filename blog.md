# Garden IoT Progress Blog

## Table of Contents
1. [Introduction](#introduction-06172026)

---

## Introduction (06/17/2026)
Hello! This file will act as an blog documenting the implementation of this project. This project will be implemented in 3 phases.

### Phase 1: Predicting plant health from biosensors
- Create and train a model to predict the health of a plant using pH, water content, light intensity, temperature (soil and ambient), and humidity.
- Create an FreeRTOS-based embedded system to take in the aformentioned parameters, run the model natively, and output the plant health on an LCD screen.
- Create a website to visualize biosensor data and predicitons from the sensor.

This phase has a deadline of **August 24th**.

### Phase 2: Predicting plant health from thermal images
- Create and train a model that can predicit the health of a plant using thermal imaging.
- Create an FreeRTOS-based embedded system to take thermal images and run a model natively to predict plant health.
- Update the website so that it can show the thermal imaging and subsequent predictions.

This phase has a deadline of **December 31st**.

### Phase 3: Prediciting harvest size from biosensors and thermal images
- Create and train a model that can using imaging and biosenor data to predict plant health and harvest size.
- Create a FreeRTOS-based embedded system that can take thermal images and biosensor data and natively run a model to predict plant health and harvest size.
- Update the website so instead of taking data from two systems, it can take the data from one.

This phase has a deadline of **July 2027**.

Clearly, there are large gaps in the deadlines. This is because the goal of this project is essentially completed through phase 1, that being:

*To gain an understanding of neural networks through implementation in PyTorch and RTOS-based embedded systems through implementation in hardware by creating a device that can take in real-world biosensor data and predict the health of a plant*

This is for educational purposes, which means I will be using tutorials that have similar implementations as mine. The models I use will use as a foundation (if not be directly copied from) tutorials online and I will use source code from FreeRTOS documentation, all of which will be directly cited in the README
: In fact, one of the tutorials has the implementation of a model that is doing the exact same thing I want my model to do in phase 1! I will be using their dataset for training rather than looking at their implementation, but it goes to show that nothing I'm doing is new. Rather, I am just trying to learn.

In this blog, I will provide updates overtime over the implementation of this project. I hope it will act as both record of my experience exploring this emerging frontier of edge AI computing and as a time capsule to show how far I've come as a computer engineer.

Starting on point one of phase one: 
> Create and train a model to predict the health of a plant using pH, EC, water content, light intensity, temperature (soil and ambient), and humidity.

I will try multiple models to test their predictive power on multivariate data for multiclass classification (i.e. which model can use multiple variables as input to predict which class a datapoint belongs to)

Below are the models I'm using and the justification contextualized to the goals of the project

| Model | Justification |
| ----------- | ----------- |
| Random Forest | Great at identifying which parameters are the most influential on the classficiation of a datapoint (Great for seeing which biosensor affects health the most) |
| Decision Tree | Simpler than random trees while still having their benefits at risk of overfitting (Simpler model is easier to operate on lower level hardware) | 
| Support Vector Machine | Perform well with high numbers of inputs and excels at mapping non linear data while being memory efficent, although they are harder to interpret (High accuracy on resource constrained hardware is what I'd want even if the model would be difficult to interpret) | 
| Gradient Boosting | Ranks the more important inputs while still being able to reliablily map non linear data (Ranking is helpful practically, so I know which inputs affect plant health the most while still being able to map non-linear data)| 
| Long Short Term Memory | Uses data from the past to influence current predictions (Plant health is usually dependent on data from the past and thus, predictions based off past data might be more accurate) | 

These models were found from basic research from forums, surface level google searches, and Gemini queries. I will use a dataset of [Estimation of Obesity Levels Based On Eating Habits and Physical Condition](http://archive.ics.uci.edu/dataset/544/estimation+of+obesity+levels+based+on+eating+habits+and+physical+condition) from the UC Irvine Machine Learning Repository to test the models out. The dataset is mixed with categorical, binary, and integer data and the target variable includes multiple classes. This structure is similar what I expect the dataset I use to train my final model, so hopefully it's a good analog.

I will also use the dataset used by Sulani Ishara in their [Plant Health Prediction with ML](https://www.kaggle.com/code/sulaniishara/plant-health-prediction-with-ml/notebook) notebook in Kaggle. They used the exact type of data I want to collect to create the exact same prediction I'd like to make. However, I'm refraining to see how they did their implementation. Remember: the point of this is for me to learn. Using a tutorial to see how to create a model is learning, but using a notebook that describes which model is the best at doing exactly what I'd like to do (while also giving me the code to implement said model) isn't really learning, it's just being lazy.

The general breakdown of this model comparison (or the most boring competition ever made) will be:
1. Prepare the datasets
   1. Graph out relationships between input values and their classification
   2. Ensure the dataset is in the correct format for model predicition
2. Test the models on the same metrics
   1. Use the same epochs, but tweak optimize the model as much as possible (change hyperparmeters, perform regularization, etc)
   2. Use confusion matricies, accuracy, and a Classification report to compare the models

With that, I should be able to pick the best model for my project!

Tutorials used:
- [Random Forest Classification](https://www.geeksforgeeks.org/machine-learning/random-forest-algorithm-in-machine-learning/)
- [Support Vector Machine Multiclass Classification](https://www.geeksforgeeks.org/machine-learning/multi-class-classification-using-support-vector-machines-svm/)
- [Gradient Boosting Classifier](https://www.geeksforgeeks.org/machine-learning/ml-gradient-boosting/)
- [Decision Tree](https://www.geeksforgeeks.org/machine-learning/decision-tree-introduction-example/)


