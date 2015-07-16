这个文件夹下是视频处理程序

一、重要文件说明
cap2_net.c		主程序
process.c		视频处理文件，用到的所有函数均在librobot/cmv.c中
colors0.txt,colors1.txt 颜色阈值文件
translate.conf		转换矩阵文件，建议保留

二、启动说明

直接./videocap -a IP，IP为你要发送的主机IP

三、简要说明

我们组的想法是将视频处理函数封装成库，所以基本所有视频处理函数都在librobot/cmv.c中