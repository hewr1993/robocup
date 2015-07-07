function result = read_threshold(filename)
%READ_THRESHOLD     read threshold from file
filename

fid = fopen(filename, 'r');
num = fscanf(fid, '%d', 1);
for i = 1 : num
    for j = 1 : 6
        result(i, j) = fscanf(fid, '%d', 1);
        if j == 1
            result(i, j) = result(i, j) - 10;
        end
        if j == 2
            result(i, j) = result(i, j) + 10;
        end
        if j == 3 || j == 5
            result(i, j) = result(i, j) - 2;
        end
        if j == 4 || j == 6
            result(i, j) = result(i, j) + 2;
        end
    end
end
fclose(fid);

%[a1 a2 a3 a4 a5 a6] = textread(filename);
%result = [a1-10 a2+10 a3 a4 a5 a6];
