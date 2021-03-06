#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <linux/user.h>
#include <gelf.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <link.h>
#define IMAGE_ADDR 0x08048000
void ptrace_readreg(int pid, struct user_regs_struct *regs);
void ptrace_writereg(int pid, struct user_regs_struct *regs);
struct user_regs_struct oldregs;
void ptrace_attach(int pid)
{
    if(ptrace(PTRACE_ATTACH, pid, NULL, NULL) < 0) {
        perror("ptrace_attach");
        exit(-1);
    }
    waitpid(pid, NULL, WUNTRACED);  
  
    ptrace_readreg(pid, &oldregs);
}
void ptrace_cont(int pid)
{
    int stat;
    if(ptrace(PTRACE_CONT, pid, NULL, NULL) < 0) {
        perror("ptrace_cont");
        exit(-1);
    }
    while(!WIFSTOPPED(stat))
        waitpid(pid, &stat, WNOHANG);
}
void ptrace_detach(int pid)
{
    ptrace_writereg(pid, &oldregs);
    if(ptrace(PTRACE_DETACH, pid, NULL, NULL) < 0) {
        perror("ptrace_detach");
        exit(-1);
    }
}
void ptrace_write(int pid, unsigned long addr, void *vptr, int len)
{
    int count;
    long word;
    count = 0;
    while(count < len) {
        memcpy(&word, vptr + count, sizeof(word));
        word = ptrace(PTRACE_POKETEXT, pid, addr + count, word);
        count += 4;
        if(errno != 0)
            printf("ptrace_write failed\t %ld\n", addr + count);
    }
}
void ptrace_read(int pid, unsigned long addr, void *vptr, int len)
{
    int i,count;
    long word;
    unsigned long *ptr = (unsigned long *)vptr;
    i = count = 0;
    while (count < len) {
        word = ptrace(PTRACE_PEEKTEXT, pid, addr + count, NULL);
        count += 4;
        ptr[i++] = word;
    }
}
char * ptrace_readstr(int pid, unsigned long addr)
{
    char *str = (char *) malloc(64);
    int i,count;
    long word;
    char *pa;
    i = count = 0;
    pa = (char *)&word;
    while(i <= 60) {
        word = ptrace(PTRACE_PEEKTEXT, pid, addr + count, NULL);
        count += 4;
        if (pa[0] == '\0') {
            str[i] = '\0';
        break;
        }
        else
            str[i++] = pa[0];
        if (pa[1] == '\0') {
            str[i] = '\0';
            break;
        }
        else
            str[i++] = pa[1];
        if (pa[2] == '\0') {
            str[i] = '\0';
            break;
        }
        else
            str[i++] = pa[2];
        if (pa[3] == '\0') {
            str[i] = '\0';
            break;
        }
        else
            str[i++] = pa[3];
    }
  
    return str;
}
void ptrace_readreg(int pid, struct user_regs_struct *regs)
{
    if(ptrace(PTRACE_GETREGS, pid, NULL, regs))
        printf("*** ptrace_readreg error ***\n");
   
    return ;
}
void ptrace_writereg(int pid, struct user_regs_struct *regs)
{
    if(ptrace(PTRACE_SETREGS, pid, NULL, regs))
        printf("*** ptrace_writereg error ***\n");
   
    return ;
}
void * ptrace_push(int pid, void *paddr, int size)
{
    unsigned long esp;
    struct user_regs_struct regs;
    ptrace_readreg(pid, &regs);
    esp = regs.esp;
    esp -= size;
    esp = esp - esp % 4;
    regs.esp = esp;
    ptrace_writereg(pid, &regs);
    ptrace_write(pid, esp, paddr, size);
    return (void *)esp;
}
void ptrace_call(int pid, unsigned long addr)
{
    void *pc;
    struct user_regs_struct regs;
    int stat;
    void *pra;
    pc = (void *) 0x41414140;
    pra = ptrace_push(pid, &pc, sizeof(pc));
    ptrace_readreg(pid, &regs);
//    ptrace_readreg(pid, &oldregs);
    regs.eip = addr;
    ptrace_writereg(pid, &regs);
    ptrace_cont(pid);
    while(!WIFSIGNALED(stat))
        waitpid(pid, &stat, WNOHANG);
   
//    ptrace_writereg(pid,&oldregs);
//    ptrace_cont(pid);
}
int nchains;
unsigned long symtab,strtab,jmprel,totalrelsize,relsize,nrels,dyn_addr;
struct link_map * get_linkmap(int pid)
{
    Elf32_Ehdr *ehdr = (Elf32_Ehdr *) malloc(sizeof(Elf32_Ehdr));      
    Elf32_Phdr *phdr = (Elf32_Phdr *) malloc(sizeof(Elf32_Phdr));
    Elf32_Dyn  *dyn =  (Elf32_Dyn *) malloc(sizeof(Elf32_Dyn));
    Elf32_Word got;
    struct link_map *map = (struct link_map *)malloc(sizeof(struct link_map));
    int i = 0;
    ptrace_read(pid, IMAGE_ADDR, ehdr, sizeof(Elf32_Ehdr));
    long phdr_addr;
    phdr_addr = IMAGE_ADDR + ehdr->e_phoff;
    printf("phdr_addr\t %p\n", phdr_addr);
    ptrace_read(pid, phdr_addr, phdr, sizeof(Elf32_Phdr));
    while(phdr->p_type != PT_DYNAMIC)
        ptrace_read(pid, phdr_addr += sizeof(Elf32_Phdr), phdr,
                                                      sizeof(Elf32_Phdr));
    long dyn_addr = phdr->p_vaddr;
    printf("dyn_addr\t %p\n", dyn_addr);
    ptrace_read(pid, dyn_addr, dyn, sizeof(Elf32_Dyn));
    while(dyn->d_tag != DT_PLTGOT) {
        ptrace_read(pid, dyn_addr + i * sizeof(Elf32_Dyn), dyn, sizeof(Elf32_Dyn));
        i++;
    }
    got = (Elf32_Word)dyn->d_un.d_ptr;
    got += 4;
    printf("GOT\t\t %p\n", got);
    unsigned long map_addr;
    ptrace_read(pid,(unsigned long)(got), &map_addr, 4);
    printf("map_addr\t %p\n", map_addr);
    ptrace_read(pid, map_addr, map, sizeof(struct link_map));
   
    free(ehdr);
    free(phdr);
    free(dyn);
    return map;
}
void get_sym_info(int pid, struct link_map *lm)
{
    Elf32_Dyn *dyn = (Elf32_Dyn *) malloc(sizeof(Elf32_Dyn));
    unsigned long dyn_addr;
    int ret;
    dyn_addr = (unsigned long)lm->l_ld;
  
    ptrace_read(pid, dyn_addr, dyn, sizeof(Elf32_Dyn));
    while(dyn->d_tag != DT_NULL){
        switch(dyn->d_tag)
        {
        case DT_SYMTAB:
            symtab = dyn->d_un.d_ptr;
            //puts("DT_SYMTAB");
            break;
        case DT_STRTAB:
            strtab = dyn->d_un.d_ptr;
            //puts("DT_STRTAB");
            break;
        case DT_HASH:
            nchains = 0;
//            ptrace_read(pid, dyn->d_un.d_ptr + lm->l_addr + 4,&nchains, sizeof(nchains));
            ptrace_read(pid, dyn->d_un.d_ptr + 4,&nchains, sizeof(nchains));
            //puts("DT_HASH");
            break;
        case DT_JMPREL:
            jmprel = dyn->d_un.d_ptr;
            //puts("DT_JMPREL");
            break;
        case DT_PLTRELSZ:
            //puts("DT_PLTRELSZ");
            totalrelsize = dyn->d_un.d_val;
            break;
        case DT_RELAENT:
            relsize = dyn->d_un.d_val;
            //puts("DT_RELAENT");
            break;
        case DT_RELENT:
            relsize = dyn->d_un.d_val;
            //puts("DT_RELENT");
            break;
        }
        ptrace_read(pid, dyn_addr += sizeof(Elf32_Dyn), dyn, sizeof(Elf32_Dyn));
    }
    nrels = totalrelsize / relsize;
    free(dyn);
}
unsigned long  find_symbol_in_linkmap(int pid, struct link_map *lm, char *sym_name)
{
    Elf32_Sym *sym = (Elf32_Sym *) malloc(sizeof(Elf32_Sym));
    int i;
    char *str;
    unsigned long ret;
    get_sym_info(pid, lm);
    if(nchains<0) printf("nchains=\n",nchains);
   
    for(i = 0; i < nchains; i++) {
        ptrace_read(pid, symtab + i * sizeof(Elf32_Sym), sym, sizeof(Elf32_Sym));
        if (!sym->st_name || !sym->st_size || !sym->st_value)
            continue;
        str = (char *) ptrace_readstr(pid, strtab + sym->st_name);
        if (strcmp(str, sym_name) == 0) {
            free(str);
            str = ptrace_readstr(pid, (unsigned long)lm->l_name);
            printf("lib name [%s]\n", str);
            free(str);
            break;
        }
        free(str);
    }

    if (i == nchains)
        ret = 0;
    else
        ret =  lm->l_addr + sym->st_value;
    free(sym);
    return ret;
}
unsigned long  find_symbol(int pid, struct link_map *map, char *sym_name)
{
    struct link_map *lm = (struct link_map *) malloc(sizeof(struct link_map));
    unsigned long sym_addr;
    char *str;
  
    sym_addr = find_symbol_in_linkmap(pid, map, sym_name);
    if (sym_addr)
        return sym_addr;
    if (!map->l_next) return 0;
    ptrace_read(pid, (unsigned long)map->l_next, lm, sizeof(struct link_map));
    sym_addr = find_symbol_in_linkmap(pid, lm, sym_name);
    while(!sym_addr && lm->l_next) {
        ptrace_read(pid, (unsigned long)lm->l_next, lm, sizeof(struct link_map));
        str = ptrace_readstr(pid, (unsigned long)lm->l_name);
        if(str[0] == '\0')
            continue;
        printf("[%s]\n", str);
        free(str);
        if ((sym_addr = find_symbol_in_linkmap(pid, lm, sym_name)))
            break;
    }
    return sym_addr;
}
void get_dyn_info(int pid);
unsigned long  find_sym_in_rel(int pid, char *sym_name)
{
    Elf32_Rel *rel = (Elf32_Rel *) malloc(sizeof(Elf32_Rel));
    Elf32_Sym *sym = (Elf32_Sym *) malloc(sizeof(Elf32_Sym));
    int i;
    char *str;
    unsigned long ret;
    get_dyn_info(pid);
    for(i = 0; i< nrels ;i++) {
        ptrace_read(pid, (unsigned long)(jmprel + i * sizeof(Elf32_Rel)),
                                                                 rel, sizeof(Elf32_Rel));
        if(ELF32_R_SYM(rel->r_info)) {
            ptrace_read(pid, symtab + ELF32_R_SYM(rel->r_info) *
                                               sizeof(Elf32_Sym), sym, sizeof(Elf32_Sym));
            str = ptrace_readstr(pid, strtab + sym->st_name);
            if (strcmp(str, sym_name) == 0) {
                free(str);
                break;
            }
            free(str);
        }
    }
    if (i == nrels)
        ret = 0;
    else
        ret =  rel->r_offset;
    free(rel);
    return ret;
}
void get_dyn_info(int pid)
{
    Elf32_Dyn *dyn = (Elf32_Dyn *) malloc(sizeof(Elf32_Dyn));
    int i = 0;
    ptrace_read(pid, dyn_addr + i * sizeof(Elf32_Dyn), dyn, sizeof(Elf32_Dyn));
    i++;
    while(dyn->d_tag){
        switch(dyn->d_tag)
        {
        case DT_SYMTAB:
            puts("DT_SYMTAB");
            symtab = dyn->d_un.d_ptr;
            break;
        case DT_STRTAB:
            strtab = dyn->d_un.d_ptr;
            //puts("DT_STRTAB");
            break;
        case DT_JMPREL:
            jmprel = dyn->d_un.d_ptr;
            //puts("DT_JMPREL");
            printf("jmprel\t %p\n", jmprel);
            break;
        case DT_PLTRELSZ:
            totalrelsize = dyn->d_un.d_val;
            //puts("DT_PLTRELSZ");
            break;
        case DT_RELAENT:
            relsize = dyn->d_un.d_val;
            //puts("DT_RELAENT");
            break;
        case DT_RELENT:
            relsize = dyn->d_un.d_val;
            //puts("DT_RELENT");
            break;
        }
        ptrace_read(pid, dyn_addr + i * sizeof(Elf32_Dyn), dyn, sizeof(Elf32_Dyn));
        i++;
    }
    nrels = totalrelsize / relsize;
    free(dyn);
}
void call_dl_open(int pid, unsigned long addr, char *libname)
{
    void *pRLibName;
    struct user_regs_struct regs;
    pRLibName = ptrace_push(pid, libname, strlen(libname) + 1);
    ptrace_readreg(pid, &regs);
    regs.eax = (unsigned long) pRLibName;
    regs.ecx = 0x0;
    regs.edx = RTLD_LAZY;
    ptrace_writereg(pid, &regs);
    ptrace_call(pid, addr);
    puts("call _dl_open ok");
}
void call_mtrace(int pid,unsigned long addr)
{
    ptrace_call(pid, addr);
    puts("call mtrace ok");
}
int main(int argc, char *argv[])
{
    int pid;
    struct link_map *map;
    char sym_name[256];
    unsigned long sym_addr;
    unsigned long new_addr,old_addr,rel_addr;
/*    if((pid = fork())<0)
    {
        printf("fork is error\n");
        return 0;
    }else if(pid == 0)
    {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execl("/home/e11963/szw/hello/hello","hello",NULL);
        exit(0);
    }
   
    waitpid(pid, NULL, WUNTRACED);  
  
    ptrace_readreg(pid, &oldregs);
*/
    pid = atoi(argv[1]);
    /* ??????? */
    ptrace_attach(pid);
    map = get_linkmap(pid);                   
//    sym_addr = find_symbol(pid, map, "_dl_open");        /* call _dl_open */
//    printf("found _dl_open at addr %p\n", sym_addr);  
//    call_dl_open(pid, sym_addr, "/lib/i686/libc-2.3.2.so");    /* ???????? */  
  
    strcpy(sym_name, "mtrace");                /* intercept */
    sym_addr = find_symbol(pid, map, sym_name);
    printf("%s addr\t %p\n", sym_name, sym_addr);
    call_mtrace(pid,sym_addr);
    ptrace_detach(pid);
    exit(0);
}


