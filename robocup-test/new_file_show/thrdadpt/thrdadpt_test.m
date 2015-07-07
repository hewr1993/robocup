% thrdadpt_test.m

% Get the threshold file.
% The parameters should be saved in file './param.txt';
% to change this filename, variable 'param_path' should be changed.
% The target samples should be saved in file './samples.conf/';
% to change this filename, variable 'samples_path' should be changed.
% The result(threshold file) will be saved in './thrd.txt';
% to change this filename, variable 'target_path' should be changed.

param_path = './param.txt';
samples_path = './samples.conf';
target_path = 'thrd.txt';
original_conf = './training/green_12.conf';
original_thrd = './training/colors_12.txt';
signal_file = './test_finish.txt';

prm = read_parameters(param_path);
thrd = get_thrd(prm, samples_path, original_conf, original_thrd);
write_threshold(thrd, target_path);

fid = fopen(signal_file, 'w');
fprintf(fid, 'finish\n');
fclose(fid);
