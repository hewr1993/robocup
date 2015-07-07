function result = read_color(filename)

fid = fopen(filename, 'r');

if (fid < 0)
	sprintf('cannot open file : %s\n', filename);
end

countc = 1;
val = fscanf(fid, '%d', 1);

while(val >= 0)
	Y(countc, 1) = val;
	U(countc, 1) = fscanf(fid, '%d', 1);
	V(countc, 1) = fscanf(fid, '%d', 1);
	countc = countc + 1;
	val = fscanf(fid, '%d', 1);
end

fclose(fid);

mean_Y = mean(Y);
mean_U = mean(U);
mean_V = mean(V);
cov_Y = sqrt(cov(Y));
cov_U = sqrt(cov(U));
cov_V = sqrt(cov(V));

result = [mean_Y, mean_U, mean_V, cov_Y, cov_U, cov_V];
