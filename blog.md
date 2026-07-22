# Garden IoT Progress Blog

## Table of Contents
1. [Introduction](#introduction-06172026)
2. ["Model-Off" and FreeRTOS](#model-off-and-freertos-07202026)
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

With that, I should be able to pick the best model for my project! The code for this comparison will be located in this [file](pytorch/Model_comparison.py)

Tutorials used:
- [Random Forest Classification](https://www.geeksforgeeks.org/machine-learning/random-forest-algorithm-in-machine-learning/)
- [Support Vector Machine Multiclass Classification](https://www.geeksforgeeks.org/machine-learning/multi-class-classification-using-support-vector-machines-svm/)
- [Gradient Boosting Classifier](https://www.geeksforgeeks.org/machine-learning/ml-gradient-boosting/)
- [Decision Tree](https://www.geeksforgeeks.org/machine-learning/decision-tree-introduction-example/)


## "Model-Off" and FreeRTOS (07/20/2026)
Well uh, it's been a minute! The month long gap in the blog can be explained for 3 reasons:
1. The goals of the model comparison expanded into more than what I previously expected
2. I jumped into FreeRTOS without writing the blog

And the third is that I am both working two jobs and moving out of my apartment! So I was a little busy.

Nonetheless, I still have results to report and a deadline steadily approaching. This blog entry is going to be very long, so long that this chapter is going to have subchapters so it's easy to navigate. Each one will show a specific aspect of the project in detail and show what I learned and the tutorials I used. The subchapters are listed below.
1. [Model-Off](#model-off)
   1. [First Dataset](#first-dataset)
   2. [Second Dataset](#second-dataset)
   3. [Final Dataset](#final-dataset)
2. [FreeRTOS](#model-off-and-freertos-07202026)
   1. [Circuitry](#circuitry)
   2. [Learning FreeRTOS](#learning-freertos)
   3. [Applying FreeRTOS](#applying-freertos)

### Model-Off
 First, we can start with the comparison of models on the two datasets (Not a typo!). Just as a reminder we are comparing **Random Forest**, **Support Vector Machine**, **Decision Tree**, **Gradient Boosting**, and **Long Short Term** models.
*We will soon see that the amount of models being compared balloned quickly!*

#### First Dataset
The first dataset was the [Estimation of Obesity Levels Based On Eating Habits and Physical Condition](http://archive.ics.uci.edu/dataset/544/estimation+of+obesity+levels+based+on+eating+habits+and+physical+condition) from the UC Irvine Machine Learning Repository. Chosen for its multivariate, part-catagorical, part-numerical data format, this dataset allowed me to test out how to use these models. This is where the most experimentation and learning happened and therefore, what most of the code in the [Model_comparison.py](pytorch/Model_comparison.py) file is dedicated to. The description of the dataset is in the file itself so to save time (for both oursakes), I won't be including it here. 

I initialized the dataset, converting string data to categorical and then numerical datatypes and choosing the input and output columns. 

I am not a data scientist, but I do understand the value of understanding the data. Unless you have made the model or have researched it extensively, in this modern era of machine learning, models can act as black boxes swalling inputs and spitting outputs. Therefore, to ensure that I at least understood how the model should be interacting with the data, I created some graphs.

The first was a correlation graph to se how much (or in my case how little) each variable corrolated with each other.
![Dataset 0 correlation matrix](/pytorch/Pictures/Dataset_0_Correlation_Heatmap.png)

As you can see, not much correlation between many variables. There seemed to be something between gender and height, so I created a regression plot to see if there was anything juicy and...
![Dataset 0 Regression plot](/pytorch/Pictures/Dataset_0_Regression_plot.png)
Yeah, not really.

For my last act of amateur data scientist (*for now*), I created a profile plot just to see how the data varies amongst each other.

![Dataset 0 Regression plot](/pytorch/Pictures/Dataset_0_Profile_plot.png)

Now this data is not standardized and the categorical data (which was essientially just yes/no) as been converted into numerical data. This means that the bar at the bottom is basically all categorical data jumping between yes (1) and no(0). However, the wildly varying data for the other two variables helps show that we need to further standardize the data before we put it in the model for training. 

With this we can begin with round 1 of the Model-Off! Each model was evaluated using:
  - Confusion Matrcies which compares the true label for an input (y-axis) with the model's predicted label for the input (x-axis)
  - Accuracy as a percentage
  - Precision: the fraction of relevant instances among the retrieved instances.
   - Recall: the fraction of relevant instances that were retrieved.
   - F1-Score: The F1 score is the harmonic mean of the precision and recall and represents both precision and recall in one metric. 

I will just show the charts since I believe they speak for themselves and no individual commentary is needed. To create the models and classification metrics, I used sklearns model libraries and mlxtend.plotting/torchmetrics/sklearn.metrics respectively. All charts were plotted using my own code.

Confusion Matricies
![Dataset 0 Confusion Matrix 1](/pytorch/Pictures/Dataset_0_Confusion_matrix.png)
![Dataset 0 Confusion Matrix 2](/pytorch/Pictures/Dataset_0_Confusion_matrix(1).png)
![Dataset 0 Confusion Matrix 3](/pytorch/Pictures/Dataset_0_Confusion_matrix(2).png)

Before I show the classification metrics, you might be asking "What does 'Optimized' mean"? Well, there are many models and a similarity between some of them is that they use hyperparameters. These hyperparameters are set by the programmer and can drastically alter the preditive potential of a model. 

But, how do we know what is the best on for a dataset? Well, we can perform a Grid Search or a Random Search. Both searches go off the same idea, we provide a matrix of data with different hyperparaters that we can use for our model. Then we can compare the accuracy of the model to find the best one based on the range we've chosen. 

Random Search effectively picks the hyperparameters at random a certain number of times and picks the best one. This gets at what we're trying to do but without so having to compare every combination. With large datasets or complex models, this can be advantageous compared to Grid Searching.

As you may have surmised, Grid Searches painstakingly compare every combination of hyperparameters. This will of course always provide the best result based on the hyperparameter ranges provided, but it takes **forever** with the time increasing drastically with more choices.

However, since the models or hyperparameter ranges I'm using aren't *too* complex or deep (and because my laptop is used to me torturing it), I chose to use Grid Search.

That's what 'Optimized' means. It is the best model based on the hyperparameters I chose, and you can see some drastic improvements already in the confusion matricies.

With that out of the way, I'll now show the classification metrics and accuracy graphs.

Classification Metrics:
![Dataset 0 Classification Metrics](/pytorch/Pictures/Dataset_0_classfication_metrics.png)

Accuracy:
![Dataset 0 Classification Metrics](/pytorch/Pictures/Dataset_0_accuracy_chart.png)

Since the accuracy values become hard the differentiate, the values (in descending order) is listed below.


| Model_name | Accuracy |
| ----------- | ----------- |
| Gradient Boosting - Optimized | 0.955083 |
| Gradient Boosting | 0.955083 | 
| Random Forest - Optimized | 0.952719 | 
| Random Forest | 0.952719 | 
| Decision tree - Gini optimized | 0.936170 | 
| Decision tree - Entropy optimized | 0.936170 | 
| Support Vector Machine (One Vs All) | 0.893617 | 
| Support Vector Machine (One Vs One) | 0.893617 | 
| Decision tree - Gini | 0.633570 | 
| Decision tree - Entropy | 0.628842 | 

We can see that our original 5 models grew all the way to 10 once the dust settled. 

But keen eyed readers might notice, where's the LSTM? Well, I have unfortunate news. After tinkering with the model for a bit I was unable to get predictions that were any better than random garbage. I tried in a seperate file (which is in my Pytorch-Tutorial repository) to get the LSTM working on a different dataset. I succedded but the accuracy never got to the 90s, really staying around the 70-80 range. 

While I could spend an extra week trying to figure it out, I am on a deadline and furthermore, the model is not the entire project. The model comprises only half of the project and I didn't want to waste time chasing an accuracy that might not be there. The other models work and their accuracies are acceptable, so I cut my losses. Perhaps later I'll get the model working, but for now I chose to move on. I suppose it acted as a hidden lesson in engineering and technical development: **Sometimes your scope can't be so atomic and you have to put the project as a whole first.**

Now with all our metrics plotted, I decided to plot one more thing. I wanted to see what the models viewed as the most important features in the dataset, which is called a feature importance chart. This helps me see what some models see that others don't. I actually used a permutation importance function to find these values as the algorithm it uses is model-agnostic, is not biased when the values are unique, and evaluates the feature's actual predictive power on data the model has not seen.

Feature Importances Charts
![Dataset 0 Feature Importance Chart 1](/pytorch/Pictures/Dataset_0_feature_importance.png)

![Dataset 0 Feature Importance Chart 2](/pytorch/Pictures/Dataset_0_feature_importance(1).png)

![Dataset 0 Feature Importance Chart 3](/pytorch/Pictures/Dataset_0_feature_importance(2).png)

*And with that our round 1 winners are:*
- **1st Place: Gradient Boosting - Optimized**
- 2nd Place: Gradient Boosting
- 3rd Place: Random Forest - Optimized

Now onto dataset 2!
#### Second Dataset
Now we go to the second dataset found in Sulani Ishara's Kaggle notebook: [Plant Health Prediction with ML](https://www.kaggle.com/code/sulaniishara/plant-health-prediction-with-ml/notebook).

I'm doing the same things as I did with dataset one, so to not waste time I'll just show all the charts now with one extra. I added boxplots of the data by variable just to see how each variable differs (more amateur data science I know). Just as before, the description of each variable is in the [Model_comparison.py](pytorch/Model_comparison.py) file.

**Plotting the Dataset**

Column Distributions
![Dataset 1 Boxplot 1](/pytorch/Pictures/Dataset_1_column_distribution.png)
![Dataset 1 Boxplot 2](/pytorch/Pictures/Dataset_1_column_distribution(1).png)
![Dataset 1 Boxplot 3](/pytorch/Pictures/Dataset_1_column_distribution(2).png)

Corrolation Matrix
![Dataset 1 Corrolation Matrix](/pytorch/Pictures/Dataset_1_corrolation_heatmap.png)

Profile Plot
![Dataset 1 Profile Plot](/pytorch/Pictures/Dataset_1_profile_plpt.png)


**Evaluating the Models**

Confusion Matrcies
![Dataset 1 Confusion Matrix 1](/pytorch/Pictures/Dataset_1_Confusion_matrix.png)
![Dataset 1 Confusion Matrix 2](/pytorch/Pictures/Dataset_1_Confusion_matrix(1).png)
![Dataset 1 Confusion Matrix 3](/pytorch/Pictures/Dataset_1_Confusion_matrix(2).png)

Classification Metrics
![Dataset 1 Classification Metrics](/pytorch/Pictures/Dataset_1_classfication_metrics.png)

Accuracy Chart
![Dataset 1 Accuracy](/pytorch/Pictures/Dataset_1_accuracy_chart.png)

| Model_name | Accuracy |
| ----------- | ----------- |
| Random Forest | 0.995833 |
| Random Forest optimized | 0.995833 | 
| Gradient Boosting - Optimized | 0.995833 | 
| Gradient Boosting | 0.995833 | 
| Decision tree - Entropy optimized | 0.995833 | 
| Decision tree - Gini optimized | 0.995833 | 
| Decision tree - Gini | 0.966667 | 
| Decision tree - Entropy | 0.966667 | 
| Support Vector Machine (One Vs One) | 0.837500 | 
| Support Vector Machine (One Vs All) | 0.837500 | 

Feature Importances
![Dataset 1 Feature Importances 1](/pytorch/Pictures/Dataset_1_feature_importance.png)
![Dataset 1 Feature Importances 2](/pytorch/Pictures/Dataset_1_feature_importance(1).png)
![Dataset 1 Feature Importances 3](/pytorch/Pictures/Dataset_1_feature_importance(2).png)

Now the winners have changed! We can see that every model has improved to the point that there is now a 6-way tie between them. This is a good problem to have as it means less compute heavy models (like decision tree in comparison to random forest) can still be just as effective. 

It also shows the value in the Model-Off, no dataset is made the same. While differences between the results of round 1 and 2 are partially due to the mainly categorical data of dataset 1 and the wholly numeric data of dataset 2, it also shows that models are not static. Just becuase a dataset is similar to another (multivariate data, multivariate numeric data), does not guarantee similar results. Testing, comparison, and analysis is key to getting the most bang for my buck on this project.

You might also notice that a model and its optimized version may be tied in accuracy. This is because the grid search parameters I chose *includes* the hyperparaters I originally chose for the non-optimized model, so it's just as likely that the best model the grid search finds happens to be the one with the original hyperparameters. It is also likely that the accuracy does not change with the new hyperparameters and that the original model works just fine. That happens, but it also means we can get by with less complex models which is good for us!

*With all that said, the winners of round 2 are:*
- **Random Forest**
- Random Forest optimized
- Gradient Boosting - Optimized

Finally, onto Round 3!
#### Final Dataset

You might be saying, "Round 3? There's only two datasets!" And you would be right! However, I came to the conclusion that due to the differences in results, having a final dataset with features that match the expected sensor inputs of my final device would be helpful for two main reasons:
1. I can see which model is able to make predictions on a restricted dataset and therefore the results could be more evenly applied to what I should expect for the final product.
2. The more data a model has, the better the prediction. But I won't have as many features as I've been testing, so slimming down the feature count again better informs my final choice.

With that I took [Sulani Ishara's dataset](https://www.kaggle.com/code/sulaniishara/plant-health-prediction-with-ml/notebook) and created an input vector that only contained the soil moisture, ambient temperature, soil temperature, humidity, light intensity, and soil ph columns.

