
include config.mk
all:
#-C是指定目录
#make -C signal   

#可执行文件应该放最后
#make -C app      

#用shell命令for搞，shell里边的变量用两个$
# 对BUILD_DIR中的所有目录, 都执行make命令, 所以每个子目录都有makefile
	@for dir in $(BUILD_DIR); \
	do \
		make -C $$dir; \
	done



# 执行make clean
clean:
#-rf：删除文件夹，强制删除中间路径
	rm -rf app/link_obj app/dep nginx
	rm -rf signal/*.gch app/*.gch

