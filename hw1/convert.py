import random
file = open('test_model.obj')	#模型文件
output = open('modeldata.txt', 'w')	#将转化后数据输出到txt
line = ""
ls = []
color = []
while True :
	line = file.readline()
	if line == "" :
		break
	line.replace('\n', '')
	tmpLs = line.split(' ')
	if tmpLs[0] == 'v' :	# 处理顶点数据
		line = line[3:]
		vertexList = [5 * float(x) for x in line.split(' ') if x != '']
		tmpStr = ''
		for item in vertexList :
			tmpStr += str(item) + 'f,'
		tmpStr = tmpStr[:-1]
		ls.append(('v', tmpStr))
	elif tmpLs[0] == 'f' :	# 处理面
		faceList = [int(x) -1  for x in [en.split('//')[0] for en in tmpLs] if x.isdigit()]
		tmpStr = str(faceList[0]) + ',' + str(faceList[1]) + ',' + str(faceList[2]) + ','
		ls.append(('f', tmpStr))
file.close()
faceCount = 0	#面数量计数
for x in ls :
	if x[0] == 'v' :	
		color = [random.randint(0, 1), random.randint(0, 1), random.randint(0, 1)]	#为顶点随机生成颜色
		print("{ XMFLOAT3("+x[1]+"), XMFLOAT4("+str(color[0])+", "+str(color[1])+", "+str(color[2])+", 1.0f) },", file = output)
	elif x[0] == 'f' :
		print(x[1], file=output)
		faceCount += 1
print("#faceCount=" + str(faceCount), file = output)