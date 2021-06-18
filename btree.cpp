// 12161104 �ڹ��� �����ͺ��̽� b+tree ���� ����
#define _CRT_SECURE_NO_WARNINGS

#include <iostream>
#include <stdio.h>
#include <vector>
#include <string>
#include <cstring>
#include <stack>
#include <algorithm>
#include <cmath>

using namespace std;

enum class Header { // ��� ������ ���� enum class(mode��)
	blockSize,
	rootBID,
	depth
};

class DataEntry {
public:
	int key;
	int value;

	DataEntry(int _key, int _value) { // ������� ����鼭 key value �ֱ�
		key = _key;
		value = _value;
	}
};

class IndexEntry {
public:
	int key;
	int BIDpointer; // key�� ������ BID ������ key <= x 

	IndexEntry(int _key, int _BIDpointer) { 
		BIDpointer = _BIDpointer;
		key = _key;
	}
};

class LeafNode { // �������� key, value�� nextBID�� ����
	public:
		int nextLeafNode; // ���� ���� ��� ����Ű�� ������
		vector<DataEntry*> dataEntries;
		int location; // ��忡 ���� ������ �����Ͱ� ����Ǿ� �ִ���

	LeafNode() {
		nextLeafNode = 0;
		location = 0; // 4���� byte����
	}
};

class NonLeafNode { // non�������� pointer�� key�� ����, key n��, ������ n+1��
public:
	int BIDpointer; // ���� ���� ������
	vector<IndexEntry*> indexEntries;
	int location; // ��忡 ���� ������ �����Ͱ� ����Ǿ� �ִ���

	NonLeafNode() {
		BIDpointer = 0;
		location = 0; // 4���� byte ����
	}
};

bool compareLeaf(DataEntry* a, DataEntry* b) {
	return a->key < b->key;
}

bool compareNonLeaf(IndexEntry* a, IndexEntry* b) {
	return a->key < b->key;
}

class BTree {
public:
	FILE* pFile;
	const char* btreeFileName;
	int blockSize;
	int blockCount;

	BTree(const char* binaryFile) {
		this->btreeFileName = binaryFile;
		this->blockSize = getHeader(Header::blockSize);
		this->blockCount = getBlockCount();
	} // �����̸��� �ʱ�ȭ�ϰ�, blockSize�� �׻� �о�´�.
		// ������ ���ٸ� getHeader�� -1����

	int getBlockCount() {
		pFile = fopen(this->btreeFileName, "rb");
		if (pFile != NULL) {
			fseek(pFile, 0, SEEK_END);
			int fileSize = ftell(pFile);
			fclose(pFile);
			return (fileSize - 12) / this->blockSize; // ���������� ��������� ������ �� ������ ���´�.
		}
		return -1;
	}

	int getHeader(Header mode) { // ��� �о���� �Լ� + ���� �Բ�
		FILE* tempFile = fopen(this->btreeFileName, "rb"); // read binary
		int block[3]; // buffer
		if (tempFile != NULL) {
			fread(block, sizeof(int), 3, tempFile); // block buffer�� int ������� 3�� ������ �о����
			fclose(tempFile);
			return block[static_cast<int>(mode)]; // blockSize = 0, rootBID = 1, depth = 2
		}
		return -1;
	}

	void setHeader(int rootBID, int depth) { // blockSize�� �ȹٲ�, ����� rootBID�� depth ���� 
		pFile = fopen(this->btreeFileName, "r+b"); // write binary
		fseek(pFile, 4, SEEK_SET);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&depth, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	LeafNode*  getLeafNode(int blockID) { // leaf block ��ü�� �ܾ���� �۾�
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // �����Ҵ� �� 0���� �ʱ�ȭ
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // ���� ó������ blockLocation ��ŭ ���� ã��
		fread(bufferArray, sizeof(int), bufferSize, tempFile); // ���ۿ� �ִ� ������ŭ ��������
		bufferSize = getNumberPerNode(); // read�� �ϸ鼭 bufferSize�� ������ ������ ������ŭ �پ�� �ٽ� �ʱ�ȭ
		LeafNode* leafNode = new LeafNode(); // ������� ���� �޸�
		for (int i = 0; i < bufferSize-1; i += 2) {  // 0�� �ƴ� �����͸� insert�Ѵ�. ������ nextBID �����ϱ� ���� size-1
			if (bufferArray[i] == 0) {
				leafNode->location = i*4; // i * 2 / 8, 0���̸� ó������, 1���̸� +8byte (i�� 2�� �ö󰣴�)
				break;
			}
			else{
				DataEntry* dataEntry = new DataEntry(bufferArray[i], bufferArray[i + 1]);
				leafNode->dataEntries.push_back(dataEntry);
			}
		}
		leafNode->nextLeafNode = bufferArray[bufferSize-1]; // ���� ��
		delete[] bufferArray;
		return leafNode;
	}

	NonLeafNode*  getNonLeafNode(int blockID) { // non leaf block ��ü�� �ܾ���� �۾�
		FILE* tempFile = fopen(this->btreeFileName, "rb");
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // �����Ҵ� �� 0���� �ʱ�ȭ
		int blockLocation = getBlockOffset(blockID);
		fseek(tempFile, blockLocation, SEEK_SET); // ���� ó������ blockLocation ��ŭ ���� ã��
		fread(bufferArray, sizeof(int), bufferSize, tempFile); // ���ۿ� �ִ� ������ŭ ��������
		bufferSize = getNumberPerNode(); // read�� �ϸ鼭 bufferSize�� ������ ������ ������ŭ �پ�� �ٽ� �ʱ�ȭ
		NonLeafNode* nonLeafNode = new NonLeafNode(); // ������� ���� �޸�
		nonLeafNode->BIDpointer = bufferArray[0]; // ���� ����
		for (int i = 1; i < bufferSize; i += 2) { // 0�� �ƴ� �����͸� insert�Ѵ�.
			if (bufferArray[i] == 0) {
				nonLeafNode->location = i*4; // 4+(i - 1) / 2 * 8, 1���� ���۽����̶� 1 ����(4 +) �ѽ��� ���� 2�� ������ 8����Ʈ
				break;
			}
			else {
				IndexEntry* indexEntry = new IndexEntry(bufferArray[i], bufferArray[i + 1]);
				nonLeafNode->indexEntries.push_back(indexEntry);
			}
		}
		delete[] bufferArray;
		return nonLeafNode;
	}

	void makeNewBlock(int blockID) { // ���� ���� �������ŭ 0���� �߰��Ѵ�
		pFile = fopen(this->btreeFileName, "r+b"); // writeBinary
		int blockLocation = getBlockOffset(blockID);
		fseek(pFile, blockLocation, SEEK_SET); // ���� ó������ blockLocation ��ŭ ���� ã��
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // �����Ҵ� �� 0���� �ʱ�ȭ
		fwrite(bufferArray, sizeof(int), bufferSize, pFile); // blockSize(byte)��ŭ 0���� ä���
		blockCount++;
		delete[] bufferArray;
		fclose(pFile);
	}

	int getNumberPerNode() { // getNode���� fread�� sizeof(int)�� �ܾ�ö� ����ϴ� ���� -> 4byte�� ����ؾ��Ѵ�.
		return floor((this->blockSize)/4); // 4byte�� ���� ���� - �ܼ� blockSize�� int ������ ���� ��
	}
	
	int getEntryPerNode() { // split���� �� ���, Entry Size�� ����(8byte ������ insert�ؾ��ϹǷ�)
		return floor((this->blockSize - 4) / 8); // 8byte entry����, leaf�� nextBID, nonLeaf�� nextLevelBID 4byte ����
	}

	int getBlockOffset(int blockID) { // �ش� blockID�� ���� entry��ġ��
		return 12 + ((blockID - 1) * this->blockSize); // ��� 12byte + blockSize ex) 1������ 12byte���� ����
	}

	bool isLeafSplit(LeafNode * temp) {
		return temp->dataEntries.size() > getEntryPerNode(); // ���� ������ split�� �Ͼ���� üũ
	}
	bool isNonLeafSplit(NonLeafNode * temp) {
		return temp->indexEntries.size() > getEntryPerNode(); // 1�� entry ���ԵǸ� split�� �Ͼ���� üũ
	}
	IndexEntry*  leafSplit(int insertLocation, LeafNode * tempLeafNode) {
		int splitBlockID = this->blockCount + 1;
		LeafNode* splitLeafNode = new LeafNode(); // �� splitNode
		splitLeafNode->nextLeafNode = tempLeafNode->nextLeafNode; // ���� block�� ����Ű�� ID�� �� block����
		makeNewBlock(splitBlockID); // �� splitNode ����
		tempLeafNode->nextLeafNode = splitBlockID; // ���� ��忡 nextLeaf ����
		// nextBID ����, splitNode�� ���� data �ű��, ���� ��忡 pointer �߰��ϱ�
		int leftSize = tempLeafNode->dataEntries.size(); // ���� ��� ������, �̹� �߰��� ���¶� (���� + 1�� �߰� entry)
		// ������ �ű�� ����
		for (int i = leftSize / 2; i < leftSize; i++) {
			splitLeafNode->dataEntries.push_back(tempLeafNode->dataEntries[i]);
		} // ���� �ű�� ������ �ݺ��� ������ �����Ѵ�.
		tempLeafNode->dataEntries.erase(tempLeafNode->dataEntries.begin() + leftSize / 2, tempLeafNode->dataEntries.end());
		updateLeafData(insertLocation, tempLeafNode); // ���Ͽ� ����� ���� ��� �ۼ�(���ο��� 0���� �ʱ�ȭ�ǰ� write)
		updateLeafData(splitBlockID, splitLeafNode); // ���Ͽ� splitNode �ۼ�(���ο��� 0���� �ʱ�ȭ�ǰ� write)
		//---------------------������� ������� split -> blockCount�� �þ�� root�� depth�� ���� X
		int newIndexKey = splitLeafNode->dataEntries.front()->key; // �����Ϳ� key + blockID�� splitBlockID ���
		IndexEntry * newIndexEntry = new IndexEntry(newIndexKey, splitBlockID);

		return newIndexEntry;
	}

	IndexEntry*  nonLeafSplit(int tempUpBlockID, NonLeafNode * tempNonLeafNode) {
		int splitBlockID = this->blockCount + 1;
		makeNewBlock(splitBlockID);
		NonLeafNode* splitNonLeafNode = new NonLeafNode();
		int leftSize = tempNonLeafNode->indexEntries.size();
		IndexEntry* newIndexEntry = new IndexEntry(tempNonLeafNode->indexEntries[leftSize/2]->key, splitBlockID);
		//���� �ö� nonLeafEntry(������ ù��° key��, ������ new blockID)�� ���� ���ش� (������ ��忡 ���� �����Ƿ�)
		// �߰��� ������ ���� ���� ���� blockID�� ����������Ѵ�.
		splitNonLeafNode->BIDpointer = tempNonLeafNode->indexEntries[leftSize/2]->BIDpointer;
		//�ö󰡴� entry�� ����Ű�� �ִ� �������� blockID�� splitNode�� ����Ű�� �ö󰡴� entry�� �� block�� ����Ŵ
		for (int i = leftSize / 2+1; i < leftSize; i++) { // �ö󰡴� entry �����ϰ� �Ű��ش�.
			splitNonLeafNode->indexEntries.push_back(tempNonLeafNode->indexEntries[i]);
		}
		tempNonLeafNode->indexEntries.erase(tempNonLeafNode->indexEntries.begin() + leftSize / 2, tempNonLeafNode->indexEntries.end());
		updateNonLeafData(tempUpBlockID, tempNonLeafNode);
		updateNonLeafData(splitBlockID, splitNonLeafNode);

		return newIndexEntry;
	}

	void updateLeafData(int blockID, LeafNode* _leafNode) {
		// writeFile -> �ش� ���� �� �����, Entry�� �ش��ϴ� key value�� ��� �ٽ� �ۼ�
		pFile = fopen(this->btreeFileName, "r+b"); // write binary
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // ���ĵ� ������ write�ؾ��ϹǷ� �Ź� blockó������ wirte������.
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // �����Ҵ� �� 0���� �ʱ�ȭ
		fwrite(bufferArray, sizeof(int), bufferSize, pFile); // blockSize(byte)��ŭ 0���� �ʱ�ȭ
		delete[] bufferArray;
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // �ٽ� offset���� ���� ���ο� entry �߰� (���� 0���� �ʱ�ȭ)
		for (int i = 0; i < _leafNode->dataEntries.size(); i++) { // ���ο� entry�� �߰� +���ĵǾ� �ִ� ��Ȳ. ���� ������Ʈ
			int key = _leafNode->dataEntries[i]->key;
			int value = _leafNode->dataEntries[i]->value;
			fwrite(&key, sizeof(int), 1, pFile);
			fwrite(&value, sizeof(int), 1, pFile);
		}
		fseek(pFile, getBlockOffset(blockID + 1) - 4, SEEK_SET);
		fwrite(&_leafNode->nextLeafNode, sizeof(int), 1, pFile);
		fclose(pFile);
	}

	void updateNonLeafData(int blockID, NonLeafNode * _nonLeafNode) { // split�� �̿�(0���� �� ����� �ٽ� write)
		pFile = fopen(this->btreeFileName, "r+b"); // write binary
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // ���ĵ� ������ write�ؾ��ϹǷ� �Ź� blockó������ wirte������.
		int bufferSize = getNumberPerNode();
		int* bufferArray = new int[bufferSize](); // �����Ҵ� �� 0���� �ʱ�ȭ
		fwrite(bufferArray, sizeof(int), bufferSize, pFile); // blockSize(byte)��ŭ 0���� �ʱ�ȭ
		delete[] bufferArray;
		fseek(pFile, getBlockOffset(blockID), SEEK_SET); // �ٽ� offset���� ���� �ٽ� �ۼ�
		fwrite(&_nonLeafNode->BIDpointer, sizeof(int), 1, pFile); // nonLeafNode�� BIDPointer����
		for (int i = 0; i < _nonLeafNode->indexEntries.size(); i++) { // ���ο� entry�� �߰� +���ĵǾ� �ִ� ��Ȳ. ���� ������Ʈ
			int key = _nonLeafNode->indexEntries[i]->key;
			int BIDpointer = _nonLeafNode->indexEntries[i]->BIDpointer;
			fwrite(&key, sizeof(int), 1, pFile);
			fwrite(&BIDpointer, sizeof(int), 1, pFile);
		}
		fclose(pFile);
	}

	void creation(string fileName, int blockSize) { // ��� write (blockSize, rootBID = 1, Depth = 0)
		pFile = fopen(this->btreeFileName, "wb");
		int rootBID = 0; // �ʱ� root null
		int rootDepth = 0; // depth 0
		// fwrite(blcokSize �����ּ�, �������� ���� ũ�� byte��, ������ ������ ����, ���� ������)
		fwrite(&blockSize, sizeof(int), 1, pFile);
		fwrite(&rootBID, sizeof(int), 1, pFile);
		fwrite(&rootDepth, sizeof(int), 1, pFile);
		// ������� write
		fclose(pFile);
	}

	void insert(int key, int value) {
		if (getHeader(Header::rootBID) == 0) { // ��Ʈ�� ���� ���� setRoot(1) + block1����
			int newRoot = 1;
			setHeader(newRoot, 0); // ��Ʈ blockID 1�� update
			makeNewBlock(newRoot); // block #1 ����
		} // ������� Ʈ�� ù �ʱ�ȭ ����(�ƹ� ���� ���� ���)
		 // �ʱ�ȭ���ְ��� ���԰��� ����
		stack<int> trackID;
		trackID = searchBlock(key); // �����ؾ��� block ã�� �� ã�ƿ� ���
		int insertLocation = trackID.top();
		trackID.pop();
		LeafNode* tempLeafNode = getLeafNode(insertLocation); // �ش� ��带 ã�Ƽ� LeafNode�� ����(dataEntry + nextBID)
		DataEntry* newDataEntry = new DataEntry(key, value);
		tempLeafNode->dataEntries.push_back(newDataEntry);
		sort(tempLeafNode->dataEntries.begin(), tempLeafNode->dataEntries.end(), compareLeaf);

		if (isLeafSplit(tempLeafNode)) { // 1�� �߰��� �� blockSize�� �Ѵ� ���, ���ĵ� ����
			IndexEntry* newIndexEntry = leafSplit(insertLocation, tempLeafNode); // ���� : key�� splitblockID
			// leaf Split = split�� �� ����, nextBlockBID ����, ������ �ݾ� ������, ���Ͽ� �������, split��� write, indexEntry ��ȯ
			while(1){ // nonLeaf�� ����
				// case 1. ���� nonLeafNode ���� ��� �������ְ� ���� �� ����
				if (trackID.empty()) { 
					int nonLeafAfterSplit = this->blockCount + 1; // �� ���� ���� blockID
					makeNewBlock(nonLeafAfterSplit); // ���� ��� ����
					setHeader(nonLeafAfterSplit, getHeader(Header::depth) + 1); // ���� �������Ƿ� count�� rootBID, depth�� update
					NonLeafNode* nonLeafNode = new NonLeafNode();
					nonLeafNode->BIDpointer = insertLocation;
					nonLeafNode->indexEntries.push_back(newIndexEntry);
					updateNonLeafData(nonLeafAfterSplit, nonLeafNode); // BIDpointer�� indexEntry �Ѳ����� write
					break; // ����
				} // ������� �������� ���� ��� ��� ����, root,depth, BIDpointer�� ���ο� indexEntry write����
				//case 2. ���� nonLeafNode �ִ� ���  ���� �� split ���� �Ǵ�
				else { // stack�� �� ���� ���� ��� BID�� �ִ� ��� �ű⿡ ������ �����Ѵ�. ��� Ÿ�� �ö󰡸鼭 split �߻� �� ��� �ݺ�
					int tempUpBlockID = trackID.top();
					trackID.pop();
									//int tempUpUpBlockID = 0;
									//if (!trackID.empty()) { // �� ���� �� ����� ���� ���� Ȯ�ο�(������ split�� �װ��� insert, ������ new block)
									//	tempUpUpBlockID = trackID.top();
									//}
					NonLeafNode* tempNonLeafNode = getNonLeafNode(tempUpBlockID); // ������� ��������
					//IndexEntry* newIndexEntry = new IndexEntry(); // ���� ���� block�� ù key ���� blockID;
					// �ϴ� ������ ����� ���
					tempNonLeafNode->indexEntries.push_back(newIndexEntry); // ���� ��忡 index �߰�
					sort(tempNonLeafNode->indexEntries.begin(), tempNonLeafNode->indexEntries.end(), compareNonLeaf);
					// split �����ϸ鼭 ���� ����鸸 �ٲ�� -> ���� index�� key���� ���缭�� �־��ְ� pointer�� �ڱⰡ ����Ű���� ���
					if (isNonLeafSplit(tempNonLeafNode)) { // ���� ���� + ���Ļ��� -> ũ�Ⱑ ���� ������ split
						//split �߻� �� leafó�� �Ȱ��� 1/2 ���ְ� ������ ����� ù ��° index entry�� ���� �÷��ش�
						//nonLeaf���� ���� ���� + �� �����ٰ� insert�ϵ� ������ newBlock �����, 
						newIndexEntry = nonLeafSplit(tempUpBlockID, tempNonLeafNode); // ���� �ö󰡾��ϴ� entry�� ����, write
						insertLocation = tempUpBlockID; // �ٲ���� ���� root �����ϰ� root�� ���� ���� �ùٸ��� �����(split �Ǳ� �� blockID)
						// while�� ���鼭 ����ִٸ� case1 ���� ����, �ƴ϶�� case2�� �ͼ� ���� ��忡 ����, ��� ����
					}
					else {// �ƴϸ� ���Ե� ������� write���ֱ�
						updateNonLeafData(tempUpBlockID, tempNonLeafNode); // split �߻� ���ϸ� insert��ä�� ���� write
						break; // split �߻� ���ϹǷ� insert ����
					}
				}
			}
		}
		else { // entry 1�� �߰��ص� split �߻����� �ʴ� ��� write���ְ� ����
			updateLeafData(insertLocation, tempLeafNode);
			return;
		}
	}
	// search�� ������� ��Ʈbid �����ͼ� ��Ʈ���� ��������
	// Node * nowNode = getNode(rootBID);
	// nowNode�� pointer�� key���� �޾Ƽ� searchKey���� ū key�� ������ �ش� pointerŸ�� ��������
	// 1 3 5 7�̰� searchKey�� 4��� 5���� ���߰� 5�� pointer(3 <= x < 5)�� ���� ������
	// �������� ���� ������忡���� ���� sequential�ϰ� ã�´�.

	stack<int> searchBlock(int searchKey) { // Ʈ���� Ÿ�� �������鼭 �����Ͱ� �ִ� ���������� Ÿ�� ����. ��Ʈ�� ������� ��ƮID ����
		int curNodeID = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		int tempDepth = 0;
		// �������� blockID�� ���ͳ� ���ÿ� �����ؼ� �����ϱ�, 
		stack<int> trackID;
		trackID.push(curNodeID);
		while (treeDepth != tempDepth) { // depth�� ����������
			NonLeafNode* tempNonLeaf = getNonLeafNode(curNodeID); // ��Ʈ���� �����鼭 �����´�.
			int searchFlag = false;
			for (int i = 0; i < tempNonLeaf->indexEntries.size(); i++) { // ���� entry�� sequential scan�ϸ鼭
				int tempBID = 0;
				int tempKey = 0;
				if (i == 0) {
					tempBID = tempNonLeaf->BIDpointer;
					tempKey = tempNonLeaf->indexEntries.front()->key;
				}
				else {
					tempBID = tempNonLeaf->indexEntries[i - 1]->BIDpointer; // ���� ������ ����
					tempKey = tempNonLeaf->indexEntries[i]->key; // ���� key������ searchKey�� �� ũ�� ����key����
				}
				if (searchKey < tempKey) { // key�� ���� key���� ������ ���� �����ͷ� ��������.
					curNodeID = tempBID;
					trackID.push(curNodeID);
					tempDepth++;
					searchFlag = true;
					break; // for�� �����ϰ� ���� ���� �Ѿ��
				}
			}
			if (!searchFlag) {// ������ �� ã�� ��� ���� ������ blockID �־�����
				curNodeID = tempNonLeaf->indexEntries.back()->BIDpointer;
				trackID.push(curNodeID);
				tempDepth++;
			} // ���� depth�� �Ѿ��
		}
		return trackID;
	}

	pair<int, int> pointSearch(int searchKey) {
		// ���� �� search �� pointSearch
		stack<int> trackID;
		trackID = searchBlock(searchKey);
		int curNodeID = trackID.top();
		LeafNode* tempLeaf = getLeafNode(curNodeID); // depth�� == �������
		for (int i = 0; i < tempLeaf->dataEntries.size(); i++) {
			int tempLeafKey = tempLeaf->dataEntries[i]->key;
			int tempLeafValue = tempLeaf->dataEntries[i]->value;
			if (tempLeafKey == searchKey) {
				return make_pair(tempLeafKey, tempLeafValue);
			}
		}
		return make_pair(-1, -1);
	}

	vector<pair<int, int>> rangeSearch(int startRange, int endRange) { // 1 3 5 7 9���� 5~8 -> 5 7
		// ���� ������ ���� ��츦 ����
		stack<int> trackID;
		trackID = searchBlock(startRange);
		int curNodeID = trackID.top();
		vector<pair<int, int>> rangeResult;
		LeafNode* tempLeaf = getLeafNode(curNodeID); 
		while (1) { // ���� ���� ���� ���� �ݺ���
			for (int i = 0; i < tempLeaf->dataEntries.size(); i++) {
				int tempLeafKey = tempLeaf->dataEntries[i]->key;
				int tempLeafValue = tempLeaf->dataEntries[i]->value;
				if (tempLeafKey >= startRange && tempLeafKey <= endRange) {
					rangeResult.push_back(make_pair(tempLeafKey, tempLeafValue));
				}
				else if (tempLeafKey > endRange) {
					return rangeResult;
				}
			}
			tempLeaf = getLeafNode(tempLeaf->nextLeafNode); // ����� ������ ���� ���� �̵�
		}
		return rangeResult;
	}

	void print() { // Level 0(��Ʈ)�� level 1�� ���, depth ���ذ��鼭 leaf���� nonleaf����
		// ����1�� Leaf���� �׳� nextBID Ÿ�� �� ����ϸ� �Ǵµ� nonLeaf�� ��Ʈ ����� nextLevelBID Ÿ�� ��������� ����
		int tempRoot = getHeader(Header::rootBID);
		int treeDepth = getHeader(Header::depth);
		if (treeDepth == 0) {
			LeafNode* leafNode = getLeafNode(tempRoot);
			cout << "<0>" << "\n";
			for (int i = 0; i < leafNode->dataEntries.size(); i++) {
				int tempKey = leafNode->dataEntries[i]->key;
				cout<< tempKey << " ";
			}// ���� 0 leaf key ���� ��� & ����1�� �������� ����
		}

		else { // depth�� 0�� �ƴ� ���
			NonLeafNode* rootNode = getNonLeafNode(tempRoot); // ��Ʈ�� nonLeaf
			cout << "<0>" << "\n";
			for (int i = 0; i < rootNode->indexEntries.size(); i++) {
				int tempKey = rootNode->indexEntries[i]->key;
				cout << tempKey << " ";
			} // ���� 0 non leaf key ���� ���
			cout << "\n";

			if (treeDepth > 1) { // ���� 1�� nonLeaf�̴�. -> �� ��� ��Ʈ��忡�� BID ���鼭 ����ؾ��Ѵ�.
				NonLeafNode* firstNonLeaf = getNonLeafNode(rootNode->BIDpointer);
				for (int i = 0; i < firstNonLeaf->indexEntries.size(); i++) {
					int tempKey = firstNonLeaf->indexEntries[i]->key;
					cout << tempKey << " ";
				} // ���� ù��° ���� 1
				for (int i = 0; i < rootNode->indexEntries.size(); i++) { // ��Ʈ���� Ÿ�� ����
					int nextLevelBID = rootNode->indexEntries[i]->BIDpointer;
					NonLeafNode* tempNonLeaf = getNonLeafNode(nextLevelBID);

					for (int i = 0; i < tempNonLeaf->indexEntries.size(); i++) {
						int tempKey = tempNonLeaf->indexEntries[i]->key;
						cout << tempKey << " ";
					}
				}
			}
			else if (treeDepth == 1) { // ����1�� leafNode
				LeafNode* leafNode = getLeafNode(rootNode->BIDpointer); // ���� ���� ���� ������
				cout << "<1>" << "\n";
				while (leafNode->nextLeafNode != 0) { // ���� ������尡 ���� ������
					for (int i = 0; i < leafNode->dataEntries.size(); i++) {
						int tempKey = leafNode->dataEntries[i]->key;
						cout<< tempKey<<" ";
					}
					leafNode = getLeafNode(leafNode->nextLeafNode);
				}// ���� 1 leaf key ���� ���
			}
		}
	}
};

vector<int> fileOpen(const char* filename, string mode) {
	// mode�� ���� fileOpen, insert/range search�� key value / key key��, point search�� key �ϳ�
	vector<int> fileData;
	FILE* readFile = fopen(filename, "r");           //���� ������ ���� ����
	if (readFile != NULL)    //������ ���ȴ��� Ȯ��
	{
		char line[255]; // �ӽ� buffer ����
		while (fgets(line, sizeof(line), readFile) != NULL) { // �� �پ� ���ۿ� �о�´�
			line[strlen(line) - 1] = '\0'; // ���� ���� ����
			char* context = NULL; // ��ū�� ����
			if (line[0] == '\0') // �о�°� ���ٸ� ����
				break;
			if (mode == "s") { // point search�� ��ūȭ �ʿ����
				fileData.push_back(stoi(line));
			}
			else { // �������� ���� �ΰ��̹Ƿ� �����ֱ�
				char* ptr = strtok_s(line, ", ", &context); // [, ]�� �������� �����ֱ�
				while (ptr != NULL) {
					fileData.push_back(stoi((string)ptr));
					ptr = strtok_s(NULL, ", ", &context);
				}
			}
		} // ���������� 1���� ���Ϳ� �����
		fclose(readFile);    //���� �ݱ�
	}
	return fileData; // ���� ����
}

// test
// creation : c Btree.bin 36
// inseart : i Btree.bin sample_insertion_input.txt
// point search : s Btree.bin sample_search_input.txt result.txt
// range search : r Btree.bin sample_range_search.txt result.txt
// print : p Btree.bin result.txt
int main(int argc, char* argv[]) {

	char command = argv[1][0];
	const char* binaryFileName = argv[2];
	int blockSize = 0;
	const char* insertFileName = "";
	const char* searchFileName = "";
	const char* resultFileName = "";
	BTree* myBtree = new BTree(binaryFileName); //argument�� ���� index btree���Ϸ� btree ����
	vector<int> inputData;

	switch (command)
	{
	case 'c':
		// create index file
		blockSize = atoi(argv[3]);
		myBtree->creation(binaryFileName, blockSize); // creation�� creation�� ���� ��� �ʱ�ȭ
		break;
	case 'i':
		// insert records from [records data file], ex) records.txt
		insertFileName = argv[3];
		inputData = fileOpen(insertFileName, "i");
		for (int i = 0; i < inputData.size(); i += 2) {
			myBtree->insert(inputData[i],inputData[i + 1]);
		}
		break;
	case 's':
		// search keys in [input file] and print results to [output file]
		searchFileName = argv[3];
		resultFileName = argv[4];
		inputData = fileOpen(searchFileName, "s");
		for (int i = 0; i < inputData.size(); i++) {
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
