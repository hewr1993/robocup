function result = get_parameters(conf_dir, thrd_dir)

conf_files = dir(strcat(conf_dir, '/*.conf'));
thrd_files = dir(strcat(thrd_dir, '/*.txt'));

path_conf = strcat(conf_dir, '/');
path_thrd = strcat(thrd_dir, '/');

for i = 1 : length(conf_files)
    conf_file(i,:) = strcat('', conf_files(i).name);
    thrd_file(i,:) = strcat('', thrd_files(i).name);
end
conf_file
thrd_file

num = size(conf_file, 1);

for i = 1 : num
    thrd(:, :, i) = read_threshold(thrd_file(i, :));
end

color_num = size(thrd(:, :, 1), 1);

for color_id = 1 : color_num
    sum = 0;
    for i = 1 : num - 1
        id1 = i;
        id2 = i+1;
        param(:, :, i) = threshold_param(thrd(color_id,:,id1), thrd(color_id,:,id2), conf_file(id1, :), conf_file(id2, :));
    end
    param_color(:, :, color_id) = mean(param, 3);
end

result = param_color;
