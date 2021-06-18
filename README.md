<h1> Database assignment </h1>
<h2> Implementation of a Disk Based B+-tree </h2>
<hr>

<h2> 기능 </h2>
1. 파일 입출력 기반 B+tree [매번 bin파일에서 block단위로 읽어와서 수행 후 다시 파일에 write해준다. (메모리상 트리 X)]<br>
2. Insert<br>
3. Point Search<br>
4. Range Search<br>
5. Level 0, 1 Print<br>

<h2> Test </h2>

- creation : c btree.bin 36 <br>
blockSize 36으로 btree.bin 파일 생성

- inseart : i btree.bin sample_insertion_input.txt<br>
key, value tree insert

- point search : s btree.bin sample_search_input.txt result.txt<br>
simple Search about key

- range search : r btree.bin sample_range_search.txt result.txt<br>
range Search from start_key to end_key

- print : p btree.bin print.txt<br>
print Level 0, 1
  

