file = open('testdemo5.obj')
output = open('model.txt', 'w')

vertexList = []
normalList = []
vertexIndex = 0
faceVertexList = []
verDic = {}
vertexStrArr = []
faceStrArr = []
def getVertexIndex(vertexTup) :
	global vertexIndex
	if vertexTup in verDic :
		return verDic[vertexTup]
	verDic[vertexTup] = vertexIndex
	verInd = vertexTup[0]
	norInd = vertexTup[1]
	verStr = '{{XMFLOAT3({:f}f,{:f}f,{:f}f), XMFLOAT3({:f}f,{:f}f, {:f}f),XMFLOAT4(0.0f,0.0f,0.0f,0.0f),XMFLOAT4(1.0f,1.0f,1.0f,1.0f)}},'\
	.format(vertexList[verInd][0],vertexList[verInd][1],vertexList[verInd][2], normalList[norInd][0], normalList[norInd][1],normalList[norInd][2])
	vertexIndex += 1
	vertexStrArr.append(verStr)
	return vertexIndex - 1
while True :
	line = file.readline()
	if line == '' :
		break
	line = line.strip()
	word = [x for x in line.split(' ') if x != '']
	if len(word) == 0 :
		continue
	if word[0] == 'v' :
		vertexList.append([5 * float(x) for x in word[1:]])
		# if vertexList[-1][0] > 0 :
		# 	vertexList[-1][0] = 1
		# else :
		# 	vertexList[-1][0] = -1
		# if vertexList[-1][1] > 0 :
		# 	vertexList[-1][1] = 1
		# else :
		# 	vertexList[-1][1] = -1
		# if vertexList[-1][2] > 1 :
		# 	vertexList[-1][2] = 1
		# else :
		# 	vertexList[-1][2] = -1
	elif word[0] == 'vn' :
		normalList.append([float(x) for x  in word[1:]])
	elif word[0] == 'f' :
		faceVertex = []
		for x in word[1:] :
			ver, nor = [int(y) -1 for y in x.split('//')]
			faceVertex.append((ver,nor))
		faceVertexList.append(faceVertex)
for faceVertex in faceVertexList :
	faceVerArr = []
	for item in faceVertex :
		index = getVertexIndex(item)
		faceVerArr.append(index)
	faceStr = '{}, {}, {}, '.format(faceVerArr[0], faceVerArr[1], faceVerArr[2])
	faceStrArr.append(faceStr)
for vertexStr in vertexStrArr :
	print(vertexStr, file=output)
print('\n======================\n', file=output)
for faceStr in faceStrArr :
	print(faceStr, file=output)
print(vertexIndex)