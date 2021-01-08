#ifndef _TRACEH_
#define _TRACEH_

typedef struct link_map {
    caddr_t     l_addr;         /* Base Address of library */
#ifdef __mips__
    caddr_t     l_offs;         /* Load Offset of library */
#endif
    const char  *l_name;        /* Absolute Path to Library */
    const void  *l_ld;          /* Pointer to .dynamic in memory */
    struct link_map *l_next, *l_prev;   /* linked list of of mapped libs */
} Link_map;



void ptrace_attach(pid_t pid)
{
    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0)
    {
        error_msg("ptrace_attach error\n");
    }

    waitpid(pid, NULL, WUNTRACED);

    ptrace_getregs(pid, &oldregs);
}
//oldregs为全局变量



// =========second
struct link_map* get_linkmap(int pid){
    int i;
    Elf_Ehdr *ehdr = (Elf_Ehdr *) malloc(sizeof(Elf_Ehdr)); 
    Elf_Phdr *phdr = (Elf_Phdr *) malloc(sizeof(Elf_Phdr));
    Elf_Dyn  *dyn = (Elf_Dyn *) malloc(sizeof(Elf_Dyn));
    Elf_Addr *gotplt;

    // 读取文件头
    ptrace_getdata(pid, IMAGE_ADDR, ehdr, sizeof(Elf_Ehdr));
    // 获取program headers table的地址
    phdr_addr = IMAGE_ADDR + ehdr->e_phoff;
    // 遍历program headers table，找到.dynamic
    for (i = 0; i < ehdr->e_phnum; i++) 
    {
        ptrace_getdata(pid, phdr_addr + i * sizeof(Elf_Phdr), phdr, sizeof(Elf_Phdr));
        if (phdr->p_type == PT_DYNAMIC) 
        {
            dyn_addr = phdr->p_vaddr;
            break;
        }
    }

    if (0 == dyn_addr) 
    {
        error_msg("cannot find the address of .dynamin\n");
    } else 
    {
        printf("[+]the address of .dynamic is %p\n", (void *)dyn_addr);
    }

    // 遍历.dynamic，找到.got.plt 
    for (i = 0; i * sizeof(Elf_Dyn) <= phdr->p_memsz; i++ )
    {
        ptrace_getdata(pid, dyn_addr + i * sizeof(Elf_Dyn), dyn, sizeof(Elf_Dyn));
        if (dyn->d_tag == DT_PLTGOT) 
        {
            gotplt = (Elf_Addr *)(dyn->d_un.d_ptr);
            break;
        }
    }
    if (NULL == gotplt) 
    {
        error_msg("cannot find the address of .got.plt\n");

    }else 
    {
        printf("[+]the address of .got.plt is %p\n", gotplt);
    }

    // 获取link_map地址
    ptrace_getdata(pid, (Elf_Addr)(gotplt + 1), &lmap_addr, sizeof(Elf_Addr));
    printf("[+]the address of link_map is %p\n", (void *)lmap_addr);

    free(ehdr);
    free(phdr);
    free(dyn);

    return (struct link_map *)lmap_addr;
}

struct lmap_result *handle_one_lmap(int pid, struct link_map *lm){
    Elf_Addr dyn_addr;
    Elf_Dyn  *dyn = (Elf_Dyn *)calloc(1, sizeof(Elf_Dyn));
    struct lmap_result *lmret = NULL;

    // 符号表
    Elf_Addr    symtab;
    Dyn_Val     syment;
    Dyn_Val     symsz;
    // 字符串表
    Elf_Addr    strtab;
    // rel.plt
    Elf_Addr    jmprel;
    Dyn_Val     relpltsz;
    // rel.dyn
    Elf_Addr    reldyn;
    Dyn_Val     reldynsz;
    // size of one REL relocs or RELA relocs
    Dyn_Val     relent;
    // 每个lmap对应的库的映射基地址
    Elf_Addr    link_addr;

    link_addr = lm->l_addr;
    dyn_addr = lm->l_ld;

    ptrace_getdata(pid, dyn_addr, dyn, sizeof(Elf_Dyn));

    while(dyn->d_tag != DT_NULL)
    {
        switch(dyn->d_tag)
        {
        // 符号表
            case DT_SYMTAB:
            symtab = dyn->d_un.d_ptr;
            break;
            case DT_SYMENT:
            syment = dyn->d_un.d_val;
            break;
            case DT_SYMINSZ:
            symsz = dyn->d_un.d_val;
            break;
        // 字符串表
            case DT_STRTAB:
            strtab = dyn->d_un.d_ptr;
            break;
        // rel.plt, Address of PLT relocs
            case DT_JMPREL:
            jmprel = dyn->d_un.d_ptr;
            break;
        // rel.plt, Size in bytes of PLT relocs
            case DT_PLTRELSZ:
            relpltsz = dyn->d_un.d_val;
            break;
        // rel.dyn, Address of Rel relocs
            case DT_REL:
            case DT_RELA:
            reldyn = dyn->d_un.d_ptr;
            break;
        // rel.dyn, Size of one Rel reloc
            case DT_RELENT:
            case DT_RELAENT:
            relent = dyn->d_un.d_val;
            break;
        //rel.dyn  Total size of Rel relocs
            case DT_RELSZ:
            case DT_RELASZ:
            reldynsz = dyn->d_un.d_val;
            break;
        }
        ptrace_getdata(pid, dyn_addr += (sizeof(Elf_Dyn)/sizeof(Elf_Addr)), dyn, sizeof(Elf_Dyn));
    }
    if (0 == syment || 0 == relent)
    {
        printf("[-]Invalid ent, syment=%u, relent=%u\n", (unsigned)syment, (unsigned)relent);
        return lmret;
    }

    lmret = (struct lmap_result *)calloc(1, sizeof(struct lmap_result));
    lmret->symtab = symtab;
    lmret->strtab = strtab;
    lmret->jmprel = jmprel;
    lmret->reldyn = reldyn;
    lmret->link_addr = link_addr;
    lmret->nsymbols = symsz / syment;
    lmret->nrelplts = relpltsz / relent;
    lmret->nreldyns = reldynsz / relent;

    free(dyn);

    return lmret;
}

Elf_Addr find_symbol_in_linkmap(int pid, struct link_map *lm, char *sym_name){
    int i = 0;
    char buf[STRLEN] = {0};
    unsigned int nlen = 0;
    Elf_Addr ret;
    Elf_Sym *sym = (Elf_Sym *)malloc(sizeof(Elf_Sym)); 

    struct lmap_result *lmret = handle_one_lmap(pid, lm);
    //lmap_result结构体，包含了SYMTAB、STRTAB、RELPLT、REPLDYN等信息
    /*
    struct lmap_result 
    {
        Elf_Addr symtab;
        Elf_Addr strtab;
        Elf_Addr jmprel;
        Elf_Addr reldyn;
        uint64_t link_addr;
        uint64_t nsymbols;
        uint64_t nrelplts;
        uint64_t nreldyns;
    };
    */
    for(i = 0; i >= 0; i++) 
    {
        // 读取link_map的符号表
        ptrace_getdata(pid, lmret->symtab + i * sizeof(Elf_Sym) ,sym ,sizeof(Elf_Sym));

        // 如果全为0，是符号表的第一项
        if (!sym->st_name && !sym->st_size && !sym->st_value) 
        {
            continue;
        }
        nlen = ptrace_getstr(pid, lmret->strtab + sym->st_name, buf, 128);
        if (buf[0] && (32 > buf[0] || 127 == buf[0]) ) 
        {
            printf(">> nothing found in this so...\n\n");
            return 0;
        }

        if (strcmp(buf, sym_name) == 0) 
        {
            printf("[+]has find the symbol name: %s\n",buf);
            if(sym->st_value == 0) 
            {//如果sym->st_value值为0，代表这个符号本身就是重定向的内容  
                continue;
            }
            else 
            {// 否则说明找到了符号
                return (lmret->link_addr + sym->st_value);
            }
        }

    }
    free(sym);
    return 0;
}

lf_Addr find_symbol(int pid, Elf_Addr lm_addr, char *sym_name){
    char buf[STRLEN] = {0};
    struct link_map lmap;
    unsigned int nlen = 0;
    while (lm_addr) 
    {
        // 读取link_map结构内容
        ptrace_getdata(pid, lm_addr, &lmap, sizeof(struct link_map));
        lm_addr = (Elf_Addr)(lmap.l_next);//获取下一个link_map

        // 判断l_name是否有效
        if (0 == lmap.l_name) 
        {
            printf("[-]invalid address of l_name\n");
            continue;
        }
        nlen = ptrace_getstr(pid, (Elf_Addr)lmap.l_name, buf, 128);
        //读取so名称
        if (0 == nlen || 0 == strlen(buf)) 
        {
            printf("[-]invalud name of link_map at %p\n", (void *)lmap.l_name);
            continue;
        }
        printf(">> start search symbol in %s:\n", buf);

        Elf_Addr sym_addr = find_symbol_in_linkmap(pid, &lmap, sym_name);
        if (sym_addr) 
        {
            return sym_addr;
        }
    }

    return 0;
}

// =========third

int inject_code(pid_t pid, unsigned long dlopen_addr, char *libc_path){
    char sbuf1[STRLEN], sbuf2[STRLEN];
    struct user_regs_struct regs, saved_regs;
    int status;
    puts(">> start inject_code to call the dlopen");

    ptrace_getregs(pid, &regs);//获取所有寄存器值
    ptrace_getdata(pid, regs.rsp + STRLEN, sbuf1, sizeof(sbuf1));
    ptrace_getdata(pid, regs.rsp, sbuf2, sizeof(sbuf2));//获取栈上数据并保存在sbuf1、2

    /*用于引发SIGSEGV信号的ret内容*/
    unsigned long ret_addr = 0x666;

    ptrace_setdata(pid, regs.rsp, (char *)&ret_addr, sizeof(ret_addr));
    ptrace_setdata(pid, regs.rsp + STRLEN, libc_path, strlen(libc_path) + 1); 

    memcpy(&saved_regs, &regs, sizeof(regs));

    printf("before inject:rsp=%zx rdi=%zx rsi=%zx rip=%zx\n", regs.rsp,regs.rdi, regs.rsi, regs.rip);

    regs.rdi = regs.rsp + STRLEN;
    regs.rsi = RTLD_NOW|RTLD_GLOBAL|RTLD_NODELETE;
    regs.rip = dlopen_addr+2;

    printf("after inject:rsp=%zx rdi=%zx rsi=%zx rip=%zx\n", regs.rsp,regs.rdi, regs.rsi, regs.rip);

    if (ptrace(PTRACE_SETREGS, pid, NULL, &regs) < 0)
    {//设置寄存器
        error_msg("inject_code:PTRACE_SETREGS 1 failed!");
    }

    if (ptrace(PTRACE_CONT, pid, NULL, NULL) < 0)
    {//设置完寄存器后让目标进程继续运行
        error_msg("inject_code:PTRACE_CONT failed!");
    }


    waitpid(pid, &status, 0);//按照最后的ret指令会使得rip=0x666，从而引发SIGSEGV
    ptrace_getregs(pid, &regs);
    printf("after waitpid inject:rsp=%zx rdi=%zx rsi=%zx rip=%zx\n", regs.rsp,regs.rdi, regs.rsi, regs.rip);

    //恢复现场，恢复所有寄存器和栈上数据
    if (ptrace(PTRACE_SETREGS, pid, 0, &saved_regs) < 0)
    {
        error_msg("inject_code:PTRACE_SETREGS 2 failed!");;
    }
    ptrace_setdata(pid, saved_regs.rsp + STRLEN, sbuf1, sizeof(sbuf1));
    ptrace_setdata(pid, saved_regs.rsp, sbuf2, sizeof(sbuf2));
    puts("-----inject_code done------");
    return 0;
}

    /* 查找要被替换的函数 */
    old_sym_addr = find_symbol(pid, map, oldfunname);      
    /* 查找hook.so中hook的函数 */
    new_sym_addr = find_symbol(pid, map, newfunname);
    /* 查找__libc_dlopen_mode，并调用它加载hook.so动态链接库 */
    dlopen_addr = find_symbol(pid, map, "__libc_dlopen_mode");

    /*把hook.so动态链接库加载进target程序 */
    inject_code(pid, dlopen_addr, libpath);


// ============fourth,five
Elf_Addr find_sym_in_rel(int pid, char *sym_name){
    Elf_Rel *rel = (Elf_Rel *) malloc(sizeof(Elf_Rel));
    Elf_Sym *sym = (Elf_Sym *) malloc(sizeof(Elf_Sym));
    int i;
    char str[STRLEN] = {0};
    unsigned long ret;
    struct lmap_result *lmret = get_dyn_info(pid);

    for (i = 0; i<lmret->nrelplts; i++) 
    {
        ptrace_getdata(pid, lmret->jmprel + i*sizeof(Elf_Rela), rel, sizeof(Elf_Rela));
        ptrace_getdata(pid, lmret->symtab + ELF64_R_SYM(rel->r_info) * sizeof(Elf_Sym), sym, sizeof(Elf_Sym));
        int n = ptrace_getstr(pid, lmret->strtab + sym->st_name, str, STRLEN);
        printf("self->st_name: %s, self->r_offset = %p\n",str, rel->r_offset);
        if (strcmp(str, sym_name) == 0) 
        {
            break;
        }
    }
    if (i == lmret->nrelplts)
        ret = 0;
    else
        ret = rel->r_offset;
    free(rel);
    return ret;
}

    /* 找到旧函数在重定向表的地址 */          
    old_rel_addr = find_sym_in_rel(pid, oldfunname);

    ptrace_getdata(pid, old_rel_addr, &target_addr, sizeof(Elf_Addr));
    ptrace_setdata(pid, old_rel_addr, &new_sym_addr, sizeof(Elf_Addr));
    //修改oldfun的got表内容为newfun

    To_detach(pid);//退出并还原ptrace attach前的寄存器内容


#endif



