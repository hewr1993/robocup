function result = threshold_param(thrd1, thrd2, filename1, filename2)
%THRESHOLD_PARAM    get the paraneters of the adaptive threshold algorithm

thrd_b = 1.5;


% read feature dots
c1 = read_color(filename1);
c2 = read_color(filename2);

b = ones(1, 3);

% calc a & b
for i = 1 : 3
    a(i) = (thrd2(2*i-1) + thrd2(2*i) - thrd1(2*i-1) - thrd1(2*i)) / 2 / (c2(i) - c1(i));
    b(i) = (thrd2(2*i) - thrd2(2*i-1)) / (thrd1(2*i) - thrd1(2*i-1)) * c1(i+3) / c2(i+3) / 1;
    if b(i) < 1
        b(i) = 1;
    end
    if b(i) > thrd_b
        b(i) = thrd_b;
    end
    if b(i) < -thrd_b
        b(i) = -thrd_b;
    end
end

result = [a; b];
