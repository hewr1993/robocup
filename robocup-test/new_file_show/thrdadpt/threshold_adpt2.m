function result = threshold_adpt2(thrd1, filename1, filename2, param)
%THRESHOLD_ADPT2
%filename1  original
%filename2  target
%thrd1      threshold of original pic
%param      parameters

c1 = read_color(filename1);
c3 = read_color(filename2);

a = param(1, :);
b = param(2, :);

result = zeros(1, 6);

for i = 1 : 3
    mu(i) = (thrd1(2*i-1) + thrd1(2*i)) / 2 + a(i) * (c3(i) - c1(i));
    csigma(i) = (thrd1(2*i) - thrd1(2*i-1)) / 2 * c3(i+3) / c1(i+3) * b(i);
    result(2*i-1) = mu(i) - csigma(i);
    result(2*i) = mu(i) + csigma(i);
end

result = uint8(result);