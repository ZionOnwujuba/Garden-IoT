import torch
import numpy as np
import pandas as pd
from sklearn.model_selection import train_test_split, GridSearchCV
from sklearn.preprocessing import StandardScaler
from sklearn.metrics import accuracy_score
from sklearn.tree import DecisionTreeClassifier
import matplotlib
from executorch.exir import to_edge_transform_and_lower
from executorch.backends.xnnpack.partition.xnnpack_partitioner import XnnpackPartitioner
import sk2torch
matplotlib.use('Agg')


dataset = pd.read_csv(r"/mnt/c/Users/Ziono/OneDrive/Documents/Garden IoT project/Garden IoT/pytorch/data/plant_health_data(1).csv") # Windows path
# dataset = pd.read_csv(r"/home/zoino/Documents/Projects/Garden-IoT/pytorch/data/plant_health_data(1).csv")
# Convert str data type values to category
convert_cols = ["Plant_Health_Status"]
dataset[convert_cols] = dataset[convert_cols].apply(lambda x: x.astype('category'))

# Convert categorical data to numeric data using factorize columns
dataset[convert_cols] = dataset[convert_cols].apply(lambda x: pd.factorize(x)[0])

# Convert float64 columns and int64 columns into float32  
float64_cols = list(dataset.select_dtypes(include='float64'))
dataset[float64_cols] = dataset[float64_cols].astype('float32')

int64_cols = list(dataset.select_dtypes(include='int64'))
dataset[int64_cols] = dataset[int64_cols].astype('float32')

input_features_ph = dataset.loc[:, "Soil_Moisture":"Soil_pH"]
target_ph = dataset.loc[:, "Plant_Health_Status"]


# COnvert to tensors
X_train, X_test, y_train, y_test = train_test_split(input_features_ph, target_ph, test_size=0.2, random_state=42)

scaler = StandardScaler()
X_train = scaler.fit_transform(X_train)
X_test = scaler.transform(X_test)

# Convert to Pytorch tensors
X_train = torch.from_numpy(np.array(X_train)).type(torch.float32)
y_train = torch.from_numpy(np.array(y_train)).type(torch.float32)
X_test = torch.from_numpy(np.array(X_test)).type(torch.float32)

print("\nDecision Tree\n")
# Train using gini
clf_gini = DecisionTreeClassifier(criterion="gini", 
                                  random_state=100, 
                                  max_depth=3, 
                                  min_samples_leaf=5)

svm_param_grid = {
    'max_depth': [None, 3, 6],
    'min_samples_leaf': [5, 10, 15, 20] # Max depth of each tree
}

grid_search_gini = GridSearchCV(clf_gini, svm_param_grid, cv=5)
 
grid_search_gini.fit(X_train, y_train)

 
# Get the best model
best_rf_gini = grid_search_gini.best_estimator_

best_target_pred = best_rf_gini.predict(X_test)

print(f"Input: {X_test}\n Insput Shape: {X_test.shape}")

print("Accuracy:",accuracy_score(y_test, best_target_pred))

gini_pytorch_model = sk2torch.wrap(best_rf_gini)
gini_pytorch_model.eval()

example_inputs = (torch.zeros(240,6),)
exported_program = torch.export.export(gini_pytorch_model, example_inputs)

# Export to a basic, unoptimized edge dialect  for bare-metal MCU
edge_manager = to_edge_transform_and_lower(exported_program)

# Convert to ExecuTorch program with portable kernels
executorch_program = edge_manager.to_executorch()

# 3. Save for deployment
with open("models/model_ptes/gini_model.pte", "wb") as f:
    f.write(executorch_program.buffer)

"""
Microcontrollers like the RP2040 load models directly from flash memory 
    rather than a file system. Use a terminal command to convert your .pte 
    file into a C header array:
    bash > xxd -i "models/model_ptes/gini_model.pte" > rp2040-freertos/lib/peripheral-libs/gini_model.h
"""

