% thrdadpt_train.m

% Training to get the parameters for adaptive threshold.
% Training samples should be placed in './training/';
% to change this path, the variable 'training_path' should be changed.
% Training samples should contain '.conf' & '.txt' files.
% All the files in the folder will be read, 
% so make sure all the files in the folder are available.
% The parameters, the trainging result, will be saved in './param.txt';
% To change this path, variable 'param_path' should be changed.

training_path = './training';
param_path = './param.txt';

param = get_parameters(training_path, training_path)
write_parameters(param, param_path);