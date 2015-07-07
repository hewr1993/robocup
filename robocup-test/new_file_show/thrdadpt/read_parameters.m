function result = read_parameters(filename)

fid = fopen(filename, 'r');
param_size = zeros(1, 3);
for i = 1 : 3
    param_size(i) = fscanf(fid, '%d', 1);
end
param = zeros(param_size);
for i = 1 : param_size(1)
    for j = 1 : param_size(2)
        for k = 1 : param_size(3)
            param(i, j, k) = fscanf(fid, '%f', 1);
        end
    end
end
fclose(fid);

result = param;