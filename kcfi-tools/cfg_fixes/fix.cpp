// kcfi-tools Copyright (C) 2015 Universidade Estadual de Campinas
//
// This software was developed by Joao Moreira <joao@overdrivepizza.com>
// at Universidade Estadual de Campinas, SP, Brazil, in 2015.
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program. If not, see <http://www.gnu.org/licenses/>.

#include <stdio.h>
#include <stdlib.h>
#include "../lib/CFGforCFI.h"

int main(){
  CFICFG cfi;
  cfi.loadCFG();
  cfi.loadDecls();
  CFINode n = cfi.getAsmNodeOffset("_copy_from_user");
  if(!cfi.fixIds(n.id, 0, 0x00dead02)){
    fprintf(stderr, "Problem fixing copy_from_user\n");
  }
  n = cfi.getAsmNodeOffset("_copy_to_user");
  if(!cfi.fixIds(n.id, 0, 0x00dead02)){
    fprintf(stderr, "Problem fixing _copy_to_user\n");
  }
  n = cfi.getAsmNodeOffset("copy_user_generic_unrolled");
  if(!cfi.fixIds(n.id, 0, 0x00dead02)){
    fprintf(stderr, "Problem fixing copy_user_generic_unrolled\n");
  }
  n = cfi.getAsmNodeOffset("copy_user_generic_string");
  if(!cfi.fixIds(n.id, 0, 0x00dead02)){
    fprintf(stderr, "Problem fixing copy_user_generic_string\n");
  }
  n = cfi.getAsmNodeOffset("copy_user_enhanced_fast_string");
  if(!cfi.fixIds(n.id, 0, 0x00dead02)){
    fprintf(stderr, "Problem fixing copy_user_enhanced_fast_string\n");
  }
  n = cfi.getAsmNodeOffset("__get_user_1");
  if(!cfi.fixIds(n.id, 0, 0x00dead03)){
    fprintf(stderr, "Problem fixing __get_user_1\n");
  }
  n = cfi.getAsmNodeOffset("__get_user_2");
  if(!cfi.fixIds(n.id, 0, 0x00dead03)){
    fprintf(stderr, "Problem fixing __get_user_2\n");
  }
  n = cfi.getAsmNodeOffset("__get_user_4");
  if(!cfi.fixIds(n.id, 0, 0x00dead03)){
    fprintf(stderr, "Problem fixing __get_user_4\n");
  }
  n = cfi.getAsmNodeOffset("__get_user_8");
  if(!cfi.fixIds(n.id, 0, 0x00dead03)){
    fprintf(stderr, "Problem fixing __get_user_8\n");
  }
  n = cfi.getAsmNodeOffset("bad_get_user");
  if(!cfi.fixIds(n.id, 0, 0x00dead03)){
    fprintf(stderr, "Problem fixing bad_get_user\n");
  }
  n = cfi.getAsmNodeOffset("__put_user_1");
  if(!cfi.fixNodeIds(n.id, 0, 0x00dead04)){
    fprintf(stderr, "Problem fixing __put_user_1\n");
  }
  n = cfi.getAsmNodeOffset("__put_user_2");
  if(!cfi.fixNodeIds(n.id, 0, 0x00dead04)){
    fprintf(stderr, "Problem fixing __put_user_2\n");
  }
  n = cfi.getAsmNodeOffset("__put_user_4");
  if(!cfi.fixNodeIds(n.id, 0, 0x00dead04)){
    fprintf(stderr, "Problem fixing __put_user_4\n");
  }
  n = cfi.getAsmNodeOffset("__put_user_8");
  if(!cfi.fixNodeIds(n.id, 0, 0x00dead04)){
    fprintf(stderr, "Problem fixing __put_user_8\n");
  }
  n = cfi.getAsmNodeOffset("bad_put_user");
  if(!cfi.fixIds(n.id, 0, 0x00dead04)){
    fprintf(stderr, "Problem fixing bad_put_user\n");
  }

  CFINode asmn = cfi.getAsmNodeOffset("clear_page_c_e");
  if(!asmn.id) fprintf(stderr, "Problem grabbing clear_page_c_e id\n");
  n = cfi.getAsmNodeOffset("clear_page");
  if(!n.id) fprintf(stderr, "Problem grabbing clear_page\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing copy_to_user\n");
  }
  n = cfi.getAsmNodeOffset("clear_page_c");
  if(!n.id) fprintf(stderr, "Problem grabbing clear_page\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing copy_to_user\n");
  }

  asmn = cfi.getAsmNodeOffset("gs_change");
  if(!asmn.id) fprintf(stderr, "Problem grabbing gs_change id\n");
  n = cfi.getAsmNodeOffset("native_load_gs_index");
  if(!n.id) fprintf(stderr, "Problem grabbing native_load_gs_index\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing native_load_gs_index\n");
  }

  asmn = cfi.getAsmNodeOffset("error_entry");
  if(!asmn.id) fprintf(stderr, "Problem grabbing error_entry id\n");
  n = cfi.getAsmNodeOffset("error_sti");
  if(!n.id) fprintf(stderr, "Problem grabbing error_sti\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing error_sti\n");
  }
  n = cfi.getAsmNodeOffset("error_kernelspace");
  if(!n.id) fprintf(stderr, "Problem grabbing error_kernelspace\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing error_kernelspace\n");
  }

  asmn = cfi.getAsmNodeOffset("crc_pcl");
  if(!asmn.id) fprintf(stderr, "Problem grabbing crc_pcl id\n");
  n = cfi.getAsmNodeOffset("do_return");
  if(!n.id) fprintf(stderr, "Problem grabbing do_return\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing do_return\n");
  }

  asmn = cfi.getAsmNodeOffset("copy_page_rep");
  if(!asmn.id) fprintf(stderr, "Problem grabbing copy_page_rep id\n");
  n = cfi.getAsmNodeOffset("copy_page");
  if(!n.id) fprintf(stderr, "Problem grabbing copy_page\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing copy_page\n");
  }

  asmn = cfi.getAsmNodeOffset("crc_t10dif_pcl");
  if(!asmn.id) fprintf(stderr, "Problem grabbing crc_t10dif_pcl id\n");
  n = cfi.getAsmNodeOffset("_cleanup");
  if(!n.id) fprintf(stderr, "Problem grabbing _cleanup\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing _cleanup\n");
  }

  asmn = cfi.getAsmNodeOffset("__memcpy");
  if(!asmn.id) fprintf(stderr, "Problem grabbing memcpy id\n");
  n = cfi.getAsmNodeOffset("memcpy");
  if(!n.id) n = cfi.createAsmNode("memcpy","__memcpy");
  if(!n.id) fprintf(stderr, "Problem creating memcpy\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing memcpy\n");
  }

  asmn = cfi.getAsmNodeOffset("__memset");
  if(!asmn.id) fprintf(stderr, "Problem grabbing memset id\n");
  n = cfi.getAsmNodeOffset("memset");
  if(!n.id) n = cfi.createAsmNode("memset","__memset");
  if(!n.id) fprintf(stderr, "Problem creating memset\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing memset\n");
  }

  // __entry_text_start is used to align the functions in x86/kernel/entry_64.S
  // when no extra alignment is needed, the function ends up empty and its
  // symbol is on the same place as save_paranoid, which ends up being hidden
  asmn = cfi.getAsmNodeOffset("__entry_text_start");
  if(!asmn.id) fprintf(stderr, "Problem grabbing entry_text_start id\n");
  n = cfi.getAsmNodeOffset("save_paranoid");
  if(!n.id) n = cfi.createAsmNode("save_paranoid","__entry_text_start");
  if(!n.id) fprintf(stderr, "Problem creating save_paranoid\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing save_paranoid\n");
  }

  // relocate_kernel has some magic assembly where it pushes the address of
  // the function to which it wants to call but in a relocated area and then
  // returns to it - something similar to a jmp, but to a different mem page
  asmn = cfi.getAsmNodeOffset("relocate_kernel");
  if(!asmn.id) fprintf(stderr, "Problem grabbing relocate_kernel id\n");
  n = cfi.getAsmNodeOffset("identity_mapped");
  if(!n.id) fprintf(stderr, "Problem grabbing identity_mapped\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing identity_mapped\n");
  }
  n = cfi.getAsmNodeOffset("virtual_mapped");
  if(!n.id) fprintf(stderr, "Problem grabbing virtual_mapped\n");
  if(!cfi.fixIds(n.id, 0, asmn.tail_id)){
    fprintf(stderr, "Problem fixing virtual_mapped\n");
  }

  cfi.storeCFG();
  return 0;
}
