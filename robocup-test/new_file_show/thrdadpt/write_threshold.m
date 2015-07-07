function write_threshold(thrd, filename)
% write the threshold to a file

fid = fopen(filename, 'w');
fprintf(fid, '%d\n', size(thrd, 1));
for i = 1 : size(thrd, 1)
    fprintf(fid, '%d %d %d %d %d %d\n', thrd(i,:));
end
fprintf(fid, '1 2 3 4 ');
for i = 5 : size(thrd, 1)
    fprintf(fid, '4 ');
end
fprintf(fid, '\n\n');
fprintf(fid, '5\n');
fprintf(fid, '0 0 255 255\n');
fprintf(fid, '1 0 0 255\n');
fprintf(fid, '2 255 0 0\n');
fprintf(fid, '3 0 255 0\n');
fprintf(fid, '4 255 255 255\n\n');
fprintf(fid, '80\n');
fprintf(fid, '16000 16000\n');
fclose(fid);