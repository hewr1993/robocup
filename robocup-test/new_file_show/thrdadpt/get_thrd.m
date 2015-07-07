function result = get_thrd(param, target_conf, original_conf, original_thrd)

thrd0 = read_threshold(original_thrd);

for i = 1 : size(thrd0, 1)
    thrd(i,:) = threshold_adpt2(thrd0(i,:), original_conf, target_conf, param(:,:,i));
end

result = thrd;