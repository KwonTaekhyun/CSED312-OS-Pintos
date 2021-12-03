#include "swap.h"

void swap_init(){
    // swap 영역 초기화
    swap_block_table = block_get_role(BLOCK_SWAP);
    ASSERT(swap_block_table != NULL);

    swap_table = bitmap_create(block_size(swap_block_table) / NUM_SECTOR_PER_PAGE);
    bitmap_set_all(swap_table, true);
}
void swap_in(size_t swap_index, void* addr){
    // swap_index의 swap slot에 저장된 데이터를 addr로 복사
    size_t i;
    size_t start_sector = swap_index * NUM_SECTOR_PER_PAGE;
    for (i = 0; i < NUM_SECTOR_PER_PAGE; i++) {
        block_read (swap_block_table, start_sector + i, addr + (i * BLOCK_SECTOR_SIZE));
    }

    bitmap_flip(swap_table, swap_index);
}
size_t swap_out(void* addr){
    // addr 주소가 가리키는 페이지를 스왑 파티션에 기록, 페이지를 기록한 swap slot 번호를 리턴

    // Find an available block region to use
    size_t swap_index = bitmap_scan (swap_table, 0, 1, true);

    size_t i;
    size_t start_sector = swap_index * NUM_SECTOR_PER_PAGE;
    for (i = 0; i < NUM_SECTOR_PER_PAGE; i++) {
        block_write(swap_table, start_sector + i, addr + (i * BLOCK_SECTOR_SIZE));
    }

    bitmap_flip(swap_table, swap_index);
    return swap_index;
}