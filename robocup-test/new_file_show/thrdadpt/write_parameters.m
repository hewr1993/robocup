function write_parameters(param, filename)

fid = fopen(filename, 'w');
param_size = size(param);
for i = 1 : 3
    fprintf(fid, '%d ', param_size(i));
end
fprintf(fid, '\n');

for i = 1 : param_size(1)
    for j = 1 : param_size(2)
        for k = 1 : param_size(3)
            fprintf(fid, '%f ', param(i, j, k));
        end
    end
end
fclose(fid);