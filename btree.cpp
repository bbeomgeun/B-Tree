// 12161104 박범근 데이터베이스 b+tree 구현 과제
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>
#include <queue>

using namespace std;

enum class Header { // 헤더 변수를 위한 enum class(mode용)
	blockSize,
	rootBID,
	depth
};

class BTree {
public:
	const char * btreeFileName;
	int blockSize;
	FILE* pFile;
	
	BTree(const char* binaryFile) { 
		this->btreeFileName = binaryFile;
		blockSize = getHeader(this->btreeFileName, Header::blockSize);
		cout << blockSize;
		cout << getHeader(this->btreeFileName, Header::rootBID);
		cout << getHeader(this->btreeFileName, Header::depth);
	} // 파일이름을 초기화하고, blockSize를 항상 읽어온다.
	 // 파일이 없다면 getHeader는 -1리턴

	int getHeader(const char* fileName, Header mode) { // 헤더 읽어오는 함수 + 모드와 함께
		FILE* tempFile = fopen(fileName, "rb"); // read binary
		int block[3]; // buffer
		fread(block, sizeof(int), 3, tempFile); // block buffer에 int 사이즈로 3개 데이터 읽어오기
		if (tempFile != NULL) {
			if (mode == Header::blockSize) {
				return block[0];
			}
			else if (mode == Header::rootBID) {
				return block[1];
			}
			else if (mode == Header::depth) {
				return block[2];
			}
			fclose(tempFile);
		}
	}

	void creation(string fileName, int blockSize) { // 헤더 write (blockSize, rootBID = 1, Depth = 0)
		pFile = fopen(this->btreeFileName, "wb");
		int rootBID = 0; // 초기 root null
		int rootDepth = 0; // depth 0
		// fwrite(blcokSize 시작주소, 데이터의 단위 크기 byte로, 저장할 데이터 개수, 파일 포인터)
		fwrite(&blockSize, sizeof(int),1, pFile); 
		fwrite(&rootBID, sizeof(int),1, pFile);
		fwrite(&rootDepth, sizeof(int), 1, pFile);
		// 순서대로 write
		fclose(pFile);
	}

	bool insert(int key, int rid) {
		// 
		

	}
	int* pointSearch(int searchKey) {
		// search는 헤더에서 루트bid 가져와서 루트부터 내려가자
		// Node * nowNode = getNode(rootBID);
		// nowNode의 pointer들 key들을 받아서 searchKey보다 큰 key를 만나면 해당 pointer타고 내려간다
		// 1 3 5 7이고 searchKey가 4라면 5에서 멈추고 5의 pointer(3 <= x < 5)를 통해 내려감
		// 리프까지 가고 리프노드에서도 역시 sequential하게 찾는다.

	}
	int* rangeSearch(int startRange, int endRange) {


	}
	void print() {
	
	}

	//Node* getNode(int blockID) { // block 전체를 긁어오는 작업
	//	FILE* tempFile = fopen(this->btreeFileName, "rb");
	//	int blockLocation = 12 + (blockID - 1) * 8; // 헤더 12byte + 한 블럭당 8byte ex) 1번블럭은 12byte부터 시작
	//	fseek(tempFile, blockLocation, SEEK_SET); // 파일 처음부터 blockLocation 만큼 가서 찾기
	// fread( int형 버퍼, sizeof(int), 갯수, 파일포인터)
	// cout<<ftell(tempFile) // 현재 커서의 위치를 문서의 맨 처음기준으로 말해준다.
	// fseek(tempFile, 4, 0); // 처음 위치부터 4byte 이후 위치로 이동
	// fgets(block, 4, tempFile); // 4~8byte rootBID

	//	// block을 끝까지 가져오고 4byte만큼 읽을때 0값이 있으면 끝내는 식으로
	//	// getNode를 통해 blockID에 해당하는 노드(블럭)을 다 읽어서 pointer(하위 blockID 가리키는)
	//	// key들을 저장해서 갯수랑 위치 맞게 insert하자
	//	// 만약 1 4 6있는데 value가 5이면 4와 6사이이므로 6의 pointer가 가리키는 bid로 하위레벨 이동
	//	// 그럼 또 그 bid를 이용해서 getNode하고 리프까지, (leaf의 포인터는 다른거를 가리키면
	//	//그걸로 리프인지 확인 또는 헤더에 depth 저장해논걸로 비교) 내려가면서 해당 bid들 stack에 저장할거니까 stack size로 비교해도 될듯
	//	// 이렇게 메모리상에 트리를 만들어서 split같은걸 처리하고 다시 disk에 write하고
	//	// 마지막으로 split일어나면 헤더도 고쳐주고(fseek으로)
	//}
};

vector<int> fileOpen(const char * filename, string mode) { 
	// mode에 따라서 fileOpen, insert/range search는 key value / key key고, point search는 key 하나
	vector<int> fileData;
	FILE* readFile = fopen(filename, "r");           //읽을 목적의 파일 선언
	if (readFile != NULL)    //파일이 열렸는지 확인
	{
		char line[255]; // 임시 buffer 역할
		while (fgets(line, sizeof(line), readFile) != NULL) { // 한 줄씩 버퍼에 읽어온다
			line[strlen(line) - 1] = '\0'; // 개행 문자 제거
			char* context = NULL; // 토큰용 버퍼
			if (line[0] == '\0') // 읽어온게 없다면 종료
				break;
			if (mode == "s") { // point search는 토큰화 필요없다
				fileData.push_back(stoi(line));
			}
			else { // 나머지는 값이 두개이므로 나눠주기
				char* ptr = strtok_s(line, ", ", &context); // [, ]를 기준으로 나눠주기
				while (ptr != NULL) {
					fileData.push_back(stoi((string)ptr));
					ptr = strtok_s(NULL, ", ", &context);
				}
			}
		} // 최종적으로 1차원 벡터에 저장됨
		fclose(readFile);    //파일 닫기
	}
	return fileData; // 벡터 리턴
}

// test
// creation : c Btree.bin 36
// inseart : i Btree.bin sample_insertion_input.txt
// point search : s Btree.bin sample_search_input.txt result.txt
// range search : r Btree.bin sample_range_search.txt result.txt
// print : p Btree.bin result.txt
int main(int argc, char* argv[]){

	char command = argv[1][0];
	const char* binaryFileName = argv[2];
	int blockSize = 0;
	const char * insertFileName = "";
	const char* searchFileName = "";
	const char* resultFileName = "";
	BTree* myBtree = new BTree(binaryFileName); //argument를 통해 index btree파일로 btree 생성
	vector<int> inputData;

	switch (command)
	{
	case 'c':
		// create index file
		blockSize = atoi(argv[3]);
		myBtree->creation(binaryFileName, blockSize); // creation은 creation을 통해 헤더 초기화
		break;
	case 'i':
		// insert records from [records data file], ex) records.txt
		insertFileName = argv[3];
		inputData = fileOpen(insertFileName, "i");
		for (int i = 0; i < inputData.size(); i+=2) {
			//myBtree->insert(inputData[i],inputData[i + 1]);
		}
		break;
	case 's':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		inputData = fileOpen(searchFileName, "s");
		for (int i = 0; i < inputData.size(); i ++) {
			//myBtree->pointSearch(inputData[i]);
		}
		break;
	case 'r':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		inputData = fileOpen(searchFileName, "r");
		for (int i = 0; i < inputData.size(); i += 2) {
			//myBtree->rangeSearch(inputData[i], inputData[i + 1]);
		}
		break;
	case 'p':
		// print B+-Tree structure to [output file]
		resultFileName = argv[3];
		//myBtree->print();
		break;
	}

}
