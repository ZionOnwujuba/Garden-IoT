import torch
import torch.nn as nn
from torch.utils.data import DataLoader, TensorDataset
import numpy as np
import pandas as pd
from sklearn.preprocessing import MinMaxScaler
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.ensemble import RandomForestClassifier, GradientBoostingClassifier
from sklearn.metrics import accuracy_score, classification_report
from sklearn.inspection import permutation_importance 
from sklearn.svm import SVC
from sklearn.tree import DecisionTreeClassifier, plot_tree
from torchmetrics import ConfusionMatrix
from mlxtend.plotting import plot_confusion_matrix
from tqdm.auto import tqdm
import matplotlib.pyplot as plt
import seaborn as sns
import json

"""
All data from the UC Irvine Machine Learning Repository 
    (Estimation of Obesity Levels Based On Eating Habits and Physical 
    Condition  [Dataset]. (2019). UCI Machine Learning Repository. 
    https://doi.org/10.24432/C5H31Z.)

Dataset Infromation (From the above source)
    This dataset include data for the estimation of obesity levels in 
    individuals from the countries of Mexico, Peru and Colombia, based 
    on their eating habits and physical condition. The data contains 17 
    attributes and 2111 records, the records are labeled with the class 
    variable NObesity (Obesity Level), that allows classification of the 
    data using the values of Insufficient Weight, Normal Weight, 
    Overweight Level I, Overweight Level II, Obesity Type I, Obesity 
    Type II and Obesity Type III. 77% of the data was generated synthetically 
    using the Weka tool and the SMOTE filter, 23% of the data was collected 
    directly from users through a web platform.

Variables: 
Gender
Age	
Height
Weight
family_history_with_overweight (Binary)
    => Has a family member suffered or suffers from overweight? (yes/no)
FAVC (Binary)
    => Do you eat high caloric food frequently? (yes/no)
FCVC (Integer) 
    => Do you usually eat vegetables in your meals? 
NCP (Continuous)
    => How many main meals do you have daily? 
CAEC (Categorical)		
    => Do you eat any food between meals? (no/sometimes/frequently/Always)
SMOKE (Binary)
    => Do you smoke? (yes/no)
CH2O (Continuous)		
    => How much water do you drink daily? (yes/no)
SCC (Binary)
    => Do you monitor the calories you eat daily? (yes/no)
FAF	(Continuous)
    => How often do you have physical activity? 
TUE (Integer)
    => How much time do you use technological devices such as cell phone, 
        videogames, television, computer and others?
CALC (Categorical)		
    => How often do you drink alcohol? (no/sometimes/frequently/Always)
MTRANS (Categorical) 
    => Which transportation do you usually use?	(Walking/Public_Transportation/Automobile/Motorbike)
NObeyesdad (Categorical)		
    => Obesity level 
        Target Classes: Insufficient Weight, Normal Weight, Overweight Level I, 
        Overweight Level II, Obesity Type I, Obesity Type II, and Obesity Type III
"""

# Load data from CSV file
dataset = pd.read_csv(r"/mnt/c/Users/Ziono/OneDrive/Documents/Garden IoT project/Garden IoT/pytorch/data/ObesityLevelsEstimationDataset.csv")

device = torch.device('cuda' if torch.cuda.is_available() else 'cpu')

print(f"\nDataset Features Types & shape: \n{dataset.dtypes}, {dataset.shape}\n")
print(dataset)

# Convert str data type values to category
convert_cols = ['Gender', 'family_history_with_overweight', 'FAVC', 'CAEC', 'SMOKE', 'SCC', 'CALC', 'MTRANS', 'NObeyesdad']
dataset[convert_cols] = dataset[convert_cols].apply(lambda x: x.astype('category'))

print(f"\nCategorical Conversions Dataset Features Types & shape: \n{dataset.dtypes}, {dataset.shape}\n")

# Convert categorical data to numeric data using factorize columns
dataset[convert_cols] = dataset[convert_cols].apply(lambda x: pd.factorize(x)[0])

# Convert float64 columns and int64 columns into float32  
float64_cols = list(dataset.select_dtypes(include='float64'))
dataset[float64_cols] = dataset[float64_cols].astype('float32')

int64_cols = list(dataset.select_dtypes(include='int64'))
dataset[int64_cols] = dataset[int64_cols].astype('float32')

print(dataset)

print(f"\nNumerical Conversions Dataset Features Types & shape: \n{dataset.dtypes}, {dataset.shape}\n")

# Extract relevant columns
# Gender,Age,Height,Weight,family_history_with_overweight,FAVC,FCVC,NCP,CAEC,SMOKE,CH2O,SCC,FAF,TUE,CALC,MTRANS,NObeyesdad

# Look at current dataset
input_features = dataset.loc[:, :"NObeyesdad"].drop(columns=["NObeyesdad"])
target = dataset.loc[:, "NObeyesdad"]
print(input_features, target)


"""
From GeeksForGeeks:
"A correlation heatmap is a 2D graphical representation of a correlation matrix between multiple variables. 
    It uses colored cells to indicate correlation values, making patterns and relationships within data visually
    interpretable. The color intensity of each cell represents the strength of the correlation:
        1 (or close to 1): Strong positive correlation (dark colors)
        0: No correlation (neutral colors)
        -1 (or close to -1): Strong negative correlation (light colors)"
"""
# Compute correlation matrix
co_mtx = input_features.corr(numeric_only=True)

# Print correlation matrix
print(co_mtx)

# Plot correlation heatmap
sns.heatmap(co_mtx, cmap="YlGnBu", annot=True)
plt.title("Correlation Heatmap")
plt.show(block=True)


sns.lmplot(x="Gender", y="Height", data=dataset, hue="NObeyesdad");
plt.title("Regression Plot of Gender and Height")
plt.show(block=True)


# Profile Plot
ax = dataset[["Gender", "Age", "Height", "Weight", "family_history_with_overweight", "FAVC", "FCVC","NCP","CAEC","SMOKE","CH2O","SCC","FAF","TUE","CALC","MTRANS"]].plot()
ax.legend(loc='center left', bbox_to_anchor=(1, 0.5));
plt.title("Profile Plot")

plt.show(block=True)


"Model Metrics Stats"
Metrics = {
    "accuracy": [],
    "classification_reports": {
        "precision" : [["Normal Weight"], ["Overweight Level I"], ["Overweight Level II"], ["Insufficient Weight"], ["Obesity Type I"], ["Obesity Type II"], ["Obesity Type III"]],
        "recall" : [["Normal Weight"], ["Overweight Level I"], ["Overweight Level II"], ["Insufficient Weight"], ["Obesity Type I"], ["Obesity Type II"], ["Obesity Type III"]],
        "f1-score" : [["Normal Weight"], ["Overweight Level I"], ["Overweight Level II"], ["Insufficient Weight"], ["Obesity Type I"], ["Obesity Type II"], ["Obesity Type III"]],
        "support" : [["Normal Weight"], ["Overweight Level I"], ["Overweight Level II"], ["Insufficient Weight"], ["Obesity Type I"], ["Obesity Type II"], ["Obesity Type III"]],
    },
    "Confusion_matrices": [],
    "Permuation_importance": [],
}

# Order: RF, RF optimized, SVM_OVO, SVM_OVA, Gini, ENtropy, Gini_optimized, Entropy_optomized, LSTM

"Random Forest Implementation"

"""
Tutorial and excerpt from CodeGenes.net (https://www.codegenes.net/blog/pytorch-random-forest/):
    Random Forest is an ensemble learning algorithm that combines multiple 
    decision trees. Each decision tree in the forest is trained on a random 
    subset of the training data (bootstrap sampling) and a random subset 
    of features. When making a prediction, the forest aggregates the predictions 
    of all the individual trees. For classification tasks, it selects the class 
    that gets the most votes, and for regression tasks, it calculates the average 
    of the predictions of all trees.

Random Forest is an algorithm rather than a specific model with layers and nodes
"""
target_classes = ["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"]
X_train, X_test, y_train, y_test = train_test_split(input_features, target, test_size=0.2, random_state=42)

# Standardize the features
# Standardize features by removing the mean and scaling to unit variance.
# The standard score of a sample x is calculated as: z = (x - u) / s
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Convert to Pytorch tensors
X_train = torch.from_numpy(np.array(X_train)).type(torch.float32)
y_train = torch.from_numpy(np.array(y_train)).type(torch.float32)
X_test = torch.from_numpy(np.array(X_test)).type(torch.float32)

model_list = []

print("\nRandom Forest Classifier\n")
# Random Forest Classifier
rf = RandomForestClassifier(n_estimators=100, # Amount of trees in the forest
                            random_state=42) # How the random samples are gathered
model_list.append((rf, None))
# Train the model
rf.fit(X_train, y_train)

target_pred = rf.predict(X_test)

print(f"\nTarget Prediction: {target_pred}\n")

accuracy = accuracy_score(y_test, target_pred)

"""
Classification Metrics (Definitions from Wikipedia)
Precision: the fraction of relevant instances among the retrieved instances.
    Measures the accuracy of a model's positive predictions.
        "Out of all instances the model predicted as positive, how many were actually correct?"
Recall: the fraction of relevant instances that were retrieved.
    Measures the proportion of actual positive instances a machine learning model correctly identifies
        "Out of all the actual target instances, how many did the model find?"
F1-Score: The F1 score is the harmonic mean of the precision and 
    recall. It thus symmetrically represents both precision and 
    recall in one metric. 
        The highest possible value of an F-score is 1.0, 
        indicating perfect precision and recall, and the 
        lowest possible value is 0, if the precision or 
        the recall is zero. 
Support: Amount of samples in each class
"""
classification_rep = classification_report(y_test, target_pred)

print(f"\nAccuracy for Random Forest: {accuracy:.2f}\n")
print("\nClassification Report for Random Forest:\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(target_pred).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)

"""
Permutation feature importance in scikit-learn is a model-agnostic technique 
    that measures feature significance by calculating the drop in a model's 
    performance score after randomly shuffling the values of a specific feature column

How it Works
    - A baseline score is calculated on a validation or test dataset using a trained model.
    - A single feature's column values are randomly shuffled, breaking its relationship 
        with the target variable.
    - The dataset is passed back through the model to compute a new performance score.
    - The difference between the baseline and new score determines that feature's importance.
    - This cycle is repeated n_repeats times per feature to generate robust mean and 
        standard deviation metrics.

Interpretation of Scores
    Positive Value: Shuffling decreased model accuracy. Higher values imply a heavier 
        reliance on that feature.
    Zero Value: Shuffling did not impact performance; the model completely ignores 
        this feature.
    Negative Value: Shuffling actually improved performance. This typically 
        highlights an uninformative feature in a small dataset where chance 
        dictates better predictions on noise.
"""

result = permutation_importance(
    rf, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)
Metrics["accuracy"].append({
    "Random_Forest": accuracy
})


classification_rep = classification_report(y_test, target_pred, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
# print(Metrics["classification_reports"])
Metrics["Confusion_matrices"].append(confmat_tensor)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Random Forest Confusion Matrix")

plt.show(block=True)
"""
print("\nRandom Forest (Optimized)\n")
# Parameter grid for random forest for hyperparameter tuning
rf_param_grid = {
    'n_estimators': [50, 100, 200],
    'max_depth': [None, 10, 20, 30] # Max depth of each tree
}

# Create a grid search object
grid_search_rf = GridSearchCV(rf, rf_param_grid, cv=5)
model_list.append((grid_search_rf, rf_param_grid))
 
# Fit the grid search object to the training data
grid_search_rf.fit(X_train, y_train)
 
# Get the best model
best_rf = grid_search_rf.best_estimator_

best_target_pred = best_rf.predict(X_test)

best_accuracy = accuracy_score(y_test, best_target_pred)



# Loop list, outputs Predicted (Actual) for each input
converted_correct_preds = [f"{target_classes[int(y)]} ({target_classes[int(np.array(y_test)[x])]})" for x, y in enumerate(best_target_pred) if target_classes[int(y)] == target_classes[int(np.array(y_test)[x])]]
converted_incorrect_preds = [f"{target_classes[int(y)]} ({target_classes[int(np.array(y_test)[x])]})" for x, y in enumerate(best_target_pred) if target_classes[int(y)] != target_classes[int(np.array(y_test)[x])]]
print(f"\nIncorrect Predictions: {converted_incorrect_preds}\n")

best_classification_rep = classification_report(y_test, best_target_pred)

print(f"\nAccuracy for Random Forest after hyperparameter tuning: {best_accuracy:.2f}\n")
print("\nClassification Report for Random Forest after hyperparameter tuning:\n", best_classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(best_target_pred).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)

result = permutation_importance(
    best_rf, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

Metrics["accuracy"].append({
    "Random_Forest_Optimized": best_accuracy
})
best_classification_rep = classification_report(y_test, best_target_pred, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(best_classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(best_classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(best_classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(best_classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)
"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Confusion Matrix for Random Forest after Hyperparameter tuning")

plt.show(block=True)
"""
"""
Support Vector Machine (SVM) Algorithm (From GeeksForGeeks)
 It tries to find the best boundary known as hyperplane 
    that separates different classes in the data. It is 
    useful when you want to do binary classification like 
    spam vs. not spam or cat vs. dog. 

    The key idea behind the SVM algorithm is to find the 
        hyperplane that best separates two classes by 
        maximizing the margin between them. This margin 
        is the distance from the hyperplane to the nearest 
        data points (support vectors) on each side.
"""
print("\nSupport Vector Machine\n")

# Initialize the SVM classifier with One-vs-One strategy
svm_ovo = SVC(decision_function_shape='ovo')
model_list.append((svm_ovo, None))

svm_ovo.fit(X_train, y_train)

# Predict and evaluate the One-vs-One model
y_pred_ovo = svm_ovo.predict(X_test)
print("SVM One-vs-One Accuracy:", accuracy_score(y_test, y_pred_ovo))

accuracy = accuracy_score(y_test, y_pred_ovo)

classification_rep = classification_report(y_test, y_pred_ovo)

print(f"\nAccuracy for SVM (One vs One): {accuracy:.2f}\n")
print("\nClassification Report for SVM (One vs One):\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(y_pred_ovo).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)

result = permutation_importance(
    svm_ovo, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("SVM Confusion Matrix (One vs One)")
plt.show(block=True)
"""
Metrics["accuracy"].append({
    "SVM_ovo": accuracy
})
classification_rep = classification_report(y_test, y_pred_ovo, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)

# Initialize the SVM classifier with One-vs-All strategy
svm_ova = SVC(decision_function_shape='ovr')
model_list.append((svm_ova, None))

svm_ova.fit(X_train, y_train)

# Predict and evaluate the One-vs-All model
y_pred_ova = svm_ova.predict(X_test)
print("SVM One-vs-All Accuracy:", accuracy_score(y_test, y_pred_ova))

accuracy = accuracy_score(y_test, y_pred_ova)

classification_rep = classification_report(y_test, y_pred_ova)

print(f"\nAccuracy for SVM (One vs All): {accuracy:.2f}\n")
print("\nClassification Report for SVM (One vs All):\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(y_pred_ova).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)

result = permutation_importance(
    svm_ova, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("SVM Confusion Matrix (One vs All)")
plt.show(block=True)
"""
Metrics["accuracy"].append({
    "SVM_ova": accuracy
})
classification_rep = classification_report(y_test, y_pred_ova, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)

"""
Decision tree

Decision Trees work by selecting the best attribute at each step 
    to split the data. This selection is based on statistical 
    metrics that measure data impurity or uncertainty.
        Start with the full dataset as the root node.
        Select the best feature using a splitting criterion.
        Split the dataset into subsets.
        Repeat the process recursively until stopping conditions are met.
        Assign class labels at leaf nodes.

Decision Trees select the best attributes for splits using metrics 
    like Gini Index, Entropy and Information Gain helping decide 
    the root and internal nodes. Entropy measures dataset impurity, 
    guiding the tree to choose splits that reduce uncertainty.
        Gini Index: Measures the probability of misclassifying 
        a randomly chosen element lower values are better.
        Entropy: Quantifies the uncertainty or impurity in a 
        dataset higher entropy means more disorder.
        Information Gain: Measures the reduction in entropy achieved 
        by splitting data on an attribute higher gain is preferred.
"""
print("\nDecision Tree\n")
# Train using gini
clf_gini = DecisionTreeClassifier(criterion="gini", 
                                  random_state=100, 
                                  max_depth=3, 
                                  min_samples_leaf=5)
model_list.append((clf_gini, None))

clf_gini.fit(X_train, y_train)
y_pred_gini = clf_gini.predict(X_test)

clf_entropy = DecisionTreeClassifier(criterion="entropy", 
                                     random_state=100, 
                                     max_depth=3, 
                                     min_samples_leaf=5)
model_list.append((clf_entropy, None))

clf_entropy.fit(X_train, y_train)
y_pred_entropy = clf_entropy.predict(X_test)


print("\nGini Training\n")
accuracy = accuracy_score(y_test, y_pred_gini)

classification_rep = classification_report(y_test, y_pred_gini)

print(f"\nAccuracy after Gini training: {accuracy:.2f}\n")
print("\nClassification Report after Gini training:\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(y_pred_gini).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)
result = permutation_importance(
    clf_gini, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Decision Tree Confusion Matrix for Gini Training")
plt.show(block=True)
"""
Metrics["accuracy"].append({
    "Decision_Tree_Gini": accuracy
})
classification_rep = classification_report(y_test, y_pred_gini, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)
print("\nEntropy Training\n")
accuracy = accuracy_score(y_test, y_pred_entropy)

classification_rep = classification_report(y_test, y_pred_entropy)

print(f"\nAccuracy after Entropy training: {accuracy:.2f}\n")
print("\nClassification Report after Entropy training:\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(y_pred_entropy).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)
result = permutation_importance(
    clf_entropy, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Decision Tree Confusion Matrix for Entropy Training")
plt.show(block=True)
"""
Metrics["accuracy"].append({
    "Decision_Tree_Entropy": accuracy
})
classification_rep = classification_report(y_test, y_pred_entropy, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)

svm_param_grid = {
    'max_depth': [None, 3, 6],
    'min_samples_leaf': [5, 10, 15, 20] # Max depth of each tree
}


grid_search_gini = GridSearchCV(clf_gini, svm_param_grid, cv=5)
model_list.append((grid_search_gini, svm_param_grid))
 
grid_search_gini.fit(X_train, y_train)

grid_search_entropy = GridSearchCV(clf_gini, svm_param_grid, cv=5)
model_list.append((grid_search_entropy, svm_param_grid))

grid_search_entropy.fit(X_train, y_train)

 
# Get the best model
best_rf_gini = grid_search_gini.best_estimator_
best_rf_entropy = grid_search_entropy.best_estimator_

best_target_pred = best_rf_gini.predict(X_test)

print("\nOptimized Gini Decision Tree\n")
accuracy = accuracy_score(y_test, best_target_pred)

classification_rep = classification_report(y_test, best_target_pred)

print(f"\nOptimized Accuracy after Gini training: {accuracy:.2f}\n")
print("\nOptimized Classification Report after Gini training:\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(best_target_pred).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)
result = permutation_importance(
    best_rf_gini, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)
"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Optimized Decision Tree Confusion Matrix for Gini Training")
plt.show(block=True)
"""
Metrics["accuracy"].append({
    "Optimized_Decision_Tree_Gini": accuracy
})
classification_rep = classification_report(y_test, best_target_pred, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)

best_target_pred = best_rf_entropy.predict(X_test)

print("\nOptimized Entropy Decision Tree\n")
accuracy = accuracy_score(y_test, best_target_pred)

classification_rep = classification_report(y_test, best_target_pred)

print(f"\nOptimized Accuracy after Entropy training: {accuracy:.2f}\n")
print("\nOptimized Classification Report after Entropy training:\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(best_target_pred).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)
result = permutation_importance(
    best_rf_entropy, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Optimized Decision Tree Confusion Matrix for Entropy Training")
plt.show(block=True)
"""
Metrics["accuracy"].append({
    "Optimized_Decision_Tree_Entropy": accuracy
})
classification_rep = classification_report(y_test, best_target_pred, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)


print("\nGradient Boosting\n")

"""
From GeeksForGeeks:
Gradient Boosting is a boosting algorithm and here each new model 
    is trained to minimize the loss function such as mean squared 
    error or cross-entropy of the previous model using gradient descent. 
        In each iteration the algorithm computes the gradient of the 
        loss function with respect to predictions and then trains a 
        new weak model to predict this gradient. Predictions of 
        the new model are then added to the ensemble (all models prediction) 
        and the process is repeated until a stopping criterion is met.
Steps:
    1. Sequential Learning Process
        The ensemble consists of multiple trees each trained to correct 
        the errors of the previous one. In the first iteration Tree 1 
        is trained on the original data X and the true labels Y. It makes
        predictions which are used to compute the errors.
    2. Residuals Calculation
        In the second iteration Tree 2 is trained using the feature 
        matrix X and the errors from Tree 1 as labels. This means 
        Tree 2 is trained to predict the errors of Tree 1. This 
        process continues for all the trees in the ensemble. 
        Each subsequent tree is trained to predict the errors 
        of the previous tree.
    3. Shrinkage
        After each tree is trained its predictions are shrunk by 
        multiplying them with the learning rate η which ranges 
        from 0 to 1. This prevents overfitting by ensuring each 
        tree has a smaller impact on the final model.
            Once all trees are trained predictions are made by 
            summing the contributions of all the trees. 
"""

gbc =GradientBoostingClassifier(n_estimators=300,
                                 learning_rate=0.05,
                                 random_state=42,
                                 max_features=5)
model_list.append((gbc, None))

gbc.fit(X_train, y_train)

y_pred_gbc = gbc.predict(X_test)

accuracy = accuracy_score(y_test, y_pred_gbc)

classification_rep = classification_report(y_test, y_pred_gbc)

print(f"\nAccuracy for Gradient Boosting Classifier: {accuracy:.2f}\n")
print("\nClassification Report for Gradient Boosting Classifier:\n", classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(y_pred_gbc).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)
result = permutation_importance(
    gbc, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

"""fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Gradient Boosting Classifier")
plt.show(block=True)"""

Metrics["accuracy"].append({
    "GBC": accuracy
})
classification_rep = classification_report(y_test, y_pred_gbc, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)

gbc_param_grid = {
    'n_estimators': [100, 200, 300],
    'learning_rate': [0.01, 0.05, 0.1],
    'max_features': [3, 5, 10] # number of features each tree can use for splitting
}

grid_search_gbc = GridSearchCV(gbc, gbc_param_grid, cv=5)
model_list.append((grid_search_gbc, gbc_param_grid))
 
# Fit the grid search object to the training data
grid_search_gbc.fit(X_train, y_train)
 
# Get the best model
best_gbc = grid_search_gbc.best_estimator_

best_target_pred = best_gbc.predict(X_test)

best_accuracy = accuracy_score(y_test, best_target_pred)

best_classification_rep = classification_report(y_test, best_target_pred)

print(f"\nAccuracy for Gradient Boosting after hyperparameter tuning: {best_accuracy:.2f}\n")
print("\nClassification Report for Gradient Boosting after hyperparameter tuning:\n", best_classification_rep)

confmat = ConfusionMatrix(task="multiclass", num_classes=7)

confmat_tensor = confmat(torch.from_numpy(best_target_pred).type(torch.float32), 
                         torch.from_numpy(np.array(y_test)).type(torch.float32)
)

result = permutation_importance(
    best_gbc, X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
)
importance_df = pd.DataFrame({
    'feature': input_features.columns,
    'importance_mean': result.importances_mean,
    'importance_std': result.importances_std
}).sort_values(by='importance_mean', ascending=False)
Metrics["Permuation_importance"].append(importance_df)

Metrics["accuracy"].append({
    "Gradient_Boosting_Optimized": best_accuracy
})
classification_rep = classification_report(y_test, best_target_pred, output_dict=True)
for i in range(len(Metrics["classification_reports"]["precision"])):
    Metrics["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
    Metrics["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
    Metrics["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
    Metrics["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
Metrics["Confusion_matrices"].append(confmat_tensor)
"""
fig, ax = plot_confusion_matrix(    
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.title("Gradient Boosting Classifier after Hyperparameter tuning")

plt.show(block=True)
"""
print("\nMultivariate Long Short Term Memory Model\n")



target = target.to_numpy()

# Split the data into training and testing sets
train_size = int(len(input_features) * 0.8)
train_input = input_features[:train_size]
test_input = input_features[train_size:]
train_target = target[:train_size]
test_target = target[train_size:]


print(f"\nTrain Input: {train_input}\n Test Input: {test_input}\n Train Shape: {train_input.shape}\n Length of Train input: {len(train_input)}\n")

def accuracy_fn(y_true, y_pred):
    correct= torch.eq(y_true, y_pred).sum().item() # torch.eq() calculates where two tensors are equal
    acc = (correct / len(y_pred)) * 100
    return acc

def create_sequences(input_data, target_data, seq_length):
    inp_seq = []
    tgt_seq = []
    for i in range(len(input_data) - seq_length):
        inp_seq.append(input_data[i:i + seq_length])
        tgt_seq.append(target_data[i + seq_length])
    return torch.tensor(np.array(inp_seq), dtype=torch.float32), torch.tensor(np.array(tgt_seq), dtype=torch.float32)

seq_length = 20
train_inp_seq, train_tgt_seq = create_sequences(train_input, train_target, seq_length)
test_inp_seq, test_tgt_seq = create_sequences(test_input, test_target, seq_length)

class MultivariateLSTM(nn.Module):
    def __init__(self, input_size, hidden_size, num_layers, output_size, dropout = 0.2):
        super().__init__()
        self.hidden_size = hidden_size
        self.num_layers = num_layers
        self.lstm = nn.LSTM(input_size=input_size,
                            hidden_size=hidden_size,
                            num_layers=num_layers,
                            batch_first=True)
        self.dropout = nn.Dropout(0.5)
        self.sigmoid = nn.Sigmoid()
        self.fc = nn.Linear(hidden_size, output_size)

    def forward(self, x):
        out, _ = self.lstm(x)
        out = self.sigmoid(self.fc(self.dropout(out[:, -1, :])))
        return out

input_size = train_inp_seq.shape[2]
hidden_size = 64 # Prev 8 then 50* then 64 then 128* then 64
num_layers = 2 # Prev 8 then 1* then 1 then 3 then 2
output_size = 1

batch_size = 128  # Adjusted batch size
train_dataset = TensorDataset(train_inp_seq, train_tgt_seq)
train_loader = DataLoader(train_dataset, batch_size=batch_size, shuffle=True)
test_dataset = TensorDataset(test_inp_seq, test_tgt_seq)
test_loader = DataLoader(test_dataset, batch_size=batch_size, shuffle=False)

model = MultivariateLSTM(input_size, hidden_size, num_layers, output_size).to(device)

loss_fn = nn.CrossEntropyLoss()
optimizer = torch.optim.Adam(model.parameters(), lr=0.01)
avg_train_acc = 0
avg_test_acc = 0
num_epochs = 100
for epoch in tqdm(range(num_epochs)):
    total_train_loss = 0
    train_acc = 0
    model.train()
    for batch_X, batch_y in train_loader:
        batch_X, batch_y = batch_X.to(device), batch_y.to(device)
        outputs = model(batch_X)
        train_pred = torch.round(outputs)
        optimizer.zero_grad()
        loss = loss_fn(outputs, batch_y.unsqueeze(1))
        train_acc += accuracy_fn(batch_y, train_pred.argmax(dim=1))
        
        loss.backward()
        optimizer.step()
        total_train_loss += loss.item()
    average_train_loss = total_train_loss / len(train_loader)
    avg_train_acc = train_acc / len(train_loader)
    ### Testing
    
    # Put mdoel in evaulation mode
    model.eval()

    with torch.inference_mode():
        total_test_loss = 0
        test_acc = 0
        for batch_X_test, batch_y_test in test_loader:
            batch_X_test, batch_y_test = batch_X_test.to(device), batch_y_test.to(device)
            test_outputs = model(batch_X_test)
            test_pred = torch.round(test_outputs)

            test_loss = loss_fn(test_outputs, batch_y_test.unsqueeze(dim=1))
            total_test_loss += test_loss.item()
            test_acc += accuracy_fn(batch_y_test, test_pred.argmax(dim=1))
    average_test_loss = total_test_loss / len(test_loader)
    avg_test_acc = test_acc / len(test_loader)

    # Print the test results every 10 epochs
    if epoch % 10 == 0:
        print(f"Epoch: {epoch} | Train Loss: {average_train_loss} | Test Loss: {average_test_loss:.5f} | Train Acc: {avg_train_acc:.5f} | Test Acc: {avg_test_acc:.5f}")

model.eval()
with torch.inference_mode():
    test_predictions = []
    for batch_X_pred in test_inp_seq:
        batch_X_pred = batch_X_pred.to(device).unsqueeze(0)
        pred = torch.round(model(batch_X_pred))
        test_predictions.append(torch.Tensor(pred))

test_target_seq = test_target[seq_length:]
test_target_seq.flags.writeable = True
confmat_target_tensor = torch.Tensor(test_target_seq).unsqueeze(1)

# Setup confusion matrix instance and compare predictions to targets
confmat = ConfusionMatrix(num_classes=7, task='multiclass')
confmat_tensor = confmat(preds=torch.cat(test_predictions),
                         target=confmat_target_tensor)

"""#3. Plot the confusion matric
fig, ax = plot_confusion_matrix(
    conf_mat=confmat_tensor.numpy(), # matplotlib likes working with NumPy
    class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"],
    figsize=(10, 7),
    colorbar=True
)
plt.show(block=True)"""

test_predictions = np.array(test_predictions).flatten()
test_target_seq = np.array(test_target_seq).flatten()

accuracy = accuracy_score(test_target_seq, test_predictions)

classification_rep = classification_report(test_target_seq, test_predictions)

print(f"\nAccuracy for Multivariate LSTM: {accuracy:.2f}\n")
print("\nClassification Report for Multivariate LSTM:\n", classification_rep)

Metrics["accuracy"].append({
    "LSTM": accuracy
})

Metrics["Confusion_matrices"].append(confmat_tensor)




col_names= ["Obesity Level", "Random Forest", "Random Forest optimized", "Support Vector Machine (One Vs One)", "Support Vector Machine (One Vs All)", "Decision tree - Gini", "Decision tree - Entropy", "Decision tree - Gini optimized", "Decision tree - Entropy optimized", "Gradient Boosting" , "Gradient Boosting - Optimized"]
plt_names= ["Random Forest", "Random Forest optimized", "Support Vector Machine (One Vs One)", "Support Vector Machine (One Vs All)", "Decision tree - Gini", "Decision tree - Entropy", "Decision tree - Gini optimized", "Decision tree - Entropy optimized", "Gradient Boosting", "Gradient Boosting - Optimized"]
class_names=["Normal Weight", "Overweight Level I", "Overweight Level II", "Insufficient Weight", "Obesity Type I", "Obesity Type II", "Obesity Type III"]

"""
Plots the Precision, F1-score, recall, support, Accuracy, confusion matricies, and permutation importance
of the models listed in the Metrics dictionary
"""
def model_metric_plot_tool_comp(Metrics, col_names, plt_names, class_names, x_axis):
    
    "Accuracy Plot"
    accuracy_list = [acc for dic in Metrics["accuracy"] for acc in dic.values()]
    # print(accuracy_list)

    accuracy_df = pd.DataFrame({
        "Model_name": plt_names,
        "Accuracy": accuracy_list[:10]
    }).sort_values(by="Accuracy", ascending=False)
    accuracy_df.plot.barh(x='Model_name', y='Accuracy',
             title='Accuracy', color='green')

    plt.show(block=True)
    print(f"\nAccuracy Ranking:\n{accuracy_df}")

    "Classification Report Plot"
    precision_df = pd.DataFrame(Metrics["classification_reports"]["precision"], columns=col_names)
    recall_df = pd.DataFrame(Metrics["classification_reports"]["recall"], columns=col_names)
    f1_score_df = pd.DataFrame(Metrics["classification_reports"]["f1-score"], columns=col_names)
    support_df = pd.DataFrame(Metrics["classification_reports"]["support"], columns=col_names)

    # print(precision_df)

    fig, axes = plt.subplots(nrows=2, ncols=2)

    precision_df.plot(x=x_axis, kind='bar', stacked=False,
            title='Model Precision By Class', width=0.7, ax=axes[0,0], legend=False)
    recall_df.plot(x=x_axis, kind='bar', stacked=False, width=0.7,
            title='Model Recall By Class', ax=axes[0,1], legend=False)
    f1_score_df.plot(x=x_axis, kind='bar', stacked=False, width=0.7,
            title='Model F1-Score By Class', ax=axes[1,0], legend=False)
    support_df.plot(x=x_axis, kind='bar', stacked=False, width=0.7,
            title='Model Support By Class', ax=axes[1,1], legend=False)
    axes.flatten()[-1].legend(loc='lower right')

    plt.show(block=True)

    "Confusion matrix Plots"
    fig, axes = plt.subplots(nrows=2, ncols=2)
    axes = axes.flatten()
    for index, (title, conf_mat) in enumerate(zip(plt_names[:4], Metrics["Confusion_matrices"][:4])):
        plot_confusion_matrix(    
            conf_mat=conf_mat.numpy(),
            class_names=class_names,
            axis=axes[index]
        )
        axes[index].set_title(f"{title} Confusion Matrix")
    plt.show(block=True)

    fig, axes = plt.subplots(nrows=2, ncols=2)
    axes = axes.flatten()
    for index, (title, conf_mat) in enumerate(zip(plt_names[4:9], Metrics["Confusion_matrices"][4:8])):
        plot_confusion_matrix(    
            conf_mat=conf_mat.numpy(),
            class_names=class_names,
            axis=axes[index]
        )
        axes[index].set_title(f"{title} Confusion Matrix")
    plt.show(block=True)

    plot_confusion_matrix(    
        conf_mat=Metrics["Confusion_matrices"][8].numpy(),
        class_names=class_names,
        figsize=(10, 7)
    )
    plt.title(f"{plt_names[8]} Confusion Matrix")
    plt.show(block=True)

    "Permutation Importance Plots"
    fig, axes = plt.subplots(nrows=3, ncols=1)
    axes = axes.flatten()
    for index, (ax, model) in enumerate(zip(axes, Metrics["Permuation_importance"][:3])):
        print(f"\n{plt_names[index]}: \n{model}\n")
        ax.bar(model["feature"], model["importance_mean"], xerr=model["importance_std"])
        ax.set_ylabel("Permutation Importance Score")
        ax.set_xlabel("Features")
        ax.set_title(f"{plt_names[index]} Feature Importance with Standard Deviation")
    plt.tight_layout()
    plt.show(block=True)

    fig, axes = plt.subplots(nrows=3, ncols=1)
    axes = axes.flatten()
    for index, (ax, model) in enumerate(zip(axes, Metrics["Permuation_importance"][3:6])):
        print(f"\n{plt_names[index+3]}: \n{model}\n")
        ax.bar(model["feature"], model["importance_mean"], xerr=model["importance_std"])
        ax.set_ylabel("Permutation Importance Score")
        ax.set_xlabel("Features")
        ax.set_title(f"{plt_names[index+3]} Feature Importance with Standard Deviation")
    plt.tight_layout()
    plt.show(block=True)

    fig, axes = plt.subplots(nrows=3, ncols=1)
    axes = axes.flatten()
    for index, (ax, model) in enumerate(zip(axes, Metrics["Permuation_importance"][6:])):
        print(f"\n{plt_names[index+6]}: \n{model}\n")
        ax.bar(model["feature"], model["importance_mean"], xerr=model["importance_std"])
        ax.set_ylabel("Permutation Importance Score")
        ax.set_xlabel("Features")
        ax.set_title(f"{plt_names[index+6]} Feature Importance with Standard Deviation")
    plt.tight_layout()
    plt.show(block=True)

model_metric_plot_tool_comp(Metrics, col_names, plt_names, class_names, 'Obesity Level')


"Plant health data"

dataset = pd.read_csv(r"/mnt/c/Users/Ziono/OneDrive/Documents/Garden IoT project/Garden IoT/pytorch/data/plant_health_data(1).csv")
# Convert str data type values to category
convert_cols = ["Plant_Health_Status"]
dataset[convert_cols] = dataset[convert_cols].apply(lambda x: x.astype('category'))

print(f"\nCategorical Conversions Dataset Features Types & shape: \n{dataset.dtypes}, {dataset.shape}\n")

columns_to_plot = dataset.columns[np.r_[1:13]].tolist()
dataset.plot(column=columns_to_plot, kind='line')
plt.title("Profile Plot")
plt.show(block=True)

columns_to_plot = dataset.columns[np.r_[1:5]].tolist()
dataset.boxplot(column=columns_to_plot, by='Plant_Health_Status')

# 2. Clean up titles and layout
plt.suptitle("Distribution of Columns by Category (1)", y=1.02)
plt.tight_layout()
plt.show(block=True)

columns_to_plot = dataset.columns[np.r_[5:9]].tolist()
dataset.boxplot(column=columns_to_plot, by='Plant_Health_Status')

# 2. Clean up titles and layout
plt.suptitle("Distribution of Columns by Category (2)", y=1.02)
plt.tight_layout()
plt.show(block=True)

columns_to_plot = dataset.columns[np.r_[9:13]].tolist()
dataset.boxplot(column=columns_to_plot, by='Plant_Health_Status')

# 2. Clean up titles and layout
plt.suptitle("Distribution of Columns by Category (3)", y=1.02)
plt.tight_layout()
plt.show(block=True)



print("\n\n\nPlant Health Dataset\n\n\n")
# Convert categorical data to numeric data using factorize columns
dataset[convert_cols] = dataset[convert_cols].apply(lambda x: pd.factorize(x)[0])

# Convert float64 columns and int64 columns into float32  
float64_cols = list(dataset.select_dtypes(include='float64'))
dataset[float64_cols] = dataset[float64_cols].astype('float32')

int64_cols = list(dataset.select_dtypes(include='int64'))
dataset[int64_cols] = dataset[int64_cols].astype('float32')

print(dataset)

input_features_ph = dataset.loc[:, "Plant_ID":"Electrochemical_Signal"]
target_ph = dataset.loc[:, "Plant_Health_Status"]
print(input_features_ph, target_ph)

# Compute correlation matrix
co_mtx = input_features_ph.corr(numeric_only=True)

# Print correlation matrix
print(co_mtx)

# Plot correlation heatmap
sns.heatmap(co_mtx, cmap="YlGnBu", annot=True)
plt.title("Correlation Heatmap")
plt.show(block=True)

Metrics_ph = {
    "accuracy": [],
    "classification_reports": {
        "precision" : [["Healthy"], ["Moderate Stress"], ["High Stress"]],
        "recall" : [["Healthy"], ["Moderate Stress"], ["High Stress"]],
        "f1-score" : [["Healthy"], ["Moderate Stress"], ["High Stress"]],
        "support" : [["Healthy"], ["Moderate Stress"], ["High Stress"]],
    },
    "Confusion_matrices": [],
    "Permuation_importance": [],
}

X_train, X_test, y_train, y_test = train_test_split(input_features_ph, target_ph, test_size=0.2, random_state=42)

# Standardize the features
# Standardize features by removing the mean and scaling to unit variance.
# The standard score of a sample x is calculated as: z = (x - u) / s
scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Convert to Pytorch tensors
X_train = torch.from_numpy(np.array(X_train)).type(torch.float32)
y_train = torch.from_numpy(np.array(y_train)).type(torch.float32)
X_test = torch.from_numpy(np.array(X_test)).type(torch.float32)

col_names= ["Plant Health", "Random Forest", "Random Forest optimized", "Support Vector Machine (One Vs One)", "Support Vector Machine (One Vs All)", "Decision tree - Gini", "Decision tree - Entropy", "Decision tree - Gini optimized", "Decision tree - Entropy optimized", "Gradient Boosting", "Gradient Boosting - Optimized"]
plt_names= ["Random Forest", "Random Forest optimized", "Support Vector Machine (One Vs One)", "Support Vector Machine (One Vs All)", "Decision tree - Gini", "Decision tree - Entropy", "Decision tree - Gini optimized", "Decision tree - Entropy optimized", "Gradient Boosting", "Gradient Boosting - Optimized"]
class_names=["Healthy", "Moderate Stress", "High Stress"]

print("\n\nPlat health Model Testing\n\n")
for index, model in enumerate(model_list):
    # Train the model
    print(f"\n{plt_names[index]} model under test\n")
    model_under_test = model[0]
    model_under_test.fit(X_train, y_train)

    if model[1] is not None:
        model_under_test = model_under_test.best_estimator_

    target_pred = model_under_test.predict(X_test)

    print(f"\nTarget Prediction: {target_pred}\n")

    accuracy = accuracy_score(y_test, target_pred)

    classification_rep = classification_report(y_test, target_pred)

    print(f"\nAccuracy for {plt_names[index]}: {accuracy:.2f}\n")
    print(f"\nClassification Report for {plt_names[index]}:\n", classification_rep)

    confmat = ConfusionMatrix(task="multiclass", num_classes=3)

    confmat_tensor = confmat(torch.from_numpy(target_pred).type(torch.float32), 
                            torch.from_numpy(np.array(y_test)).type(torch.float32)
    )

    result = permutation_importance(
        model[0], X_test, y_test, n_repeats=5, random_state=42, n_jobs=-1
    )
    importance_df = pd.DataFrame({
        'feature': input_features_ph.columns,
        'importance_mean': result.importances_mean,
        'importance_std': result.importances_std
    }).sort_values(by='importance_mean', ascending=False)
    Metrics_ph["Permuation_importance"].append(importance_df)
    Metrics_ph["accuracy"].append({
        f"{plt_names[index]}": accuracy
    })

    classification_rep = classification_report(y_test, target_pred, output_dict=True)
    for i in range(len(Metrics_ph["classification_reports"]["precision"])):
        Metrics_ph["classification_reports"]["precision"][i].append(classification_rep[f"{i}.0"]["precision"])
        Metrics_ph["classification_reports"]["recall"][i].append(classification_rep[f"{i}.0"]["recall"])
        Metrics_ph["classification_reports"]["f1-score"][i].append(classification_rep[f"{i}.0"]["f1-score"])
        Metrics_ph["classification_reports"]["support"][i].append(classification_rep[f"{i}.0"]["support"])
    # print(Metrics_ph["classification_reports"])
    Metrics_ph["Confusion_matrices"].append(confmat_tensor)



model_metric_plot_tool_comp(Metrics_ph, col_names, plt_names, class_names, 'Plant Health')