一、实验所需的库与开发环境
1. 建议使用助教提供的Ubuntu8.10虚拟机。
2. 所学的库可以参考FAQ，至少需要libqt3-mt、libqt3-mt和libqt3-mt-dev这三个库。

二、重要代码的说明

main.c 			主程序
handler.c 		消息解析文件
sched.c，sched.h 	决策算法
task.c 			任务层文件
motor_control.c 	底层控制文件
motor_test.c		我们自己写的底层测试程序
my_utils.c		我们自己的小工具文件
common.h		我们所有定义的结构与常量

./librobot/cmv.c	视觉部分所有处理函数库文件

./net_viewer 文件夹	实时监控程序
./vision 文件夹 	视觉处理程序部分

./vision/translate.conf	是我们的转换矩阵，精度较高，建议使用。逆向标定方法，详见顾荣辉报告

三、主程序运行
只需要直接运行即可，-a 后可跟接受信息的主机IP

四、一些建议

1. 大家在写代码时可以参考cli.c里的函数命名和格式规范
输出信息尽量使用my_utils.c里的函数mylogfd

2. 可以使用git进行代码版本维护。git使用简要说明：
	1) 大家可以自己创建一个分支后，在自己的分支下开发
	2) 安装git：sudo apt-get install git
	3) 进入工程主目录：cd robocup-src
	4) 显示当前所有分支：git branch
	5) 添加分支 yourname：git branch yourname
	6) 进入 yourname 分支：git checkout yourname
3. 代码写完后，可以打包：make handin ID=学号
