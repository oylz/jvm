#ifndef _HSPERFDATA_PARSER_
#define _HSPERFDATA_PARSER_
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <string>

#pragma pack(1)
struct perfdata_header{
	uint32_t magic_; // magic number - 0xcafec0c0
	unsigned char byte_order_;   // big_endian == 0, little_endian == 1
	unsigned char major_;   // major version numbers
	unsigned char minor_;   // minor version numbers
	// ReservedByte byte   // used as accessible_ flag at performance data V2
};

struct buffer_prologue{
	unsigned char accessible_;  // accessible_ flag at performance data V2
	int32_t used_; // number of PerfData memory bytes used
	int32_t overflow_; // number of bytes of overflow
	int64_t modify_time_; // time stamp of the last structural modification
	int32_t entry_offset_; // offset of the first PerfDataEntry
	int32_t num_entries_; // number of allocated PerfData entries
};

struct entry_header{
	int32_t entry_length_; // entry length in bytes
	int32_t name_offset_; // offset to entry name, relative to start of entry
	int32_t vector_length_; // length of the vector. If 0, then scalar.
	unsigned char data_type_;  // JNI field descriptor type
	unsigned char flags_;  // miscellaneous attribute flags 0x01 - supported
	unsigned char data_units_;  // unit of measure attribute
	unsigned char data_var_;  // variability attribute
	int32_t data_offset_; // offset to data item, relative to start of entry.
};
#pragma pack()

class hsperfdata_parser{
private:
    static bool is_generation(const std::string &name){
        int pos = (int)name.find("sun.gc.generation");
        if(pos < 0){
            return false;
        }
        return true;
    }
public:
    static void parse(const std::string &filepath){
    	// read a snapshot into memory
        FILE *fl = fopen(filepath.c_str(), "rb");
        if(fl == NULL){
            fprintf(stderr, "error open file:%s\n", filepath.c_str());
            return;
        }
        fseek(fl, 0, SEEK_END);
        int len = ftell(fl);
        fseek(fl, 0, SEEK_SET);
        unsigned char *buf = new unsigned char[len];
        memset(buf, 0, len);
        fread(buf, 1, len, fl);
        fclose(fl);


        int pos = 0;
    	perfdata_header header;
        memcpy(&header, buf+pos, sizeof(header));
        pos += sizeof(header);
    	if(header.magic_ != 0xc0c0feca) {
            fprintf(stderr, "illegal magic %x\n", header.magic_);
    		return;
    	}
    	// only support 2.0 perf data buffers.
    	if(!(header.major_ == 2 && header.minor_ == 0)){
    		fprintf(stderr, "unsupported version %x.%x\n", header.major_, header.minor_);
            return;
    	}
        if(header.byte_order_ != 1){
            fprintf(stderr, "unsupport bigendian\n");
            return;
        }    
   
    	buffer_prologue prologue;
        memcpy(&prologue, buf+pos, sizeof(prologue));
           
        pos = prologue.entry_offset_;
   
    	for(int i = 0; i < prologue.num_entries_; i++){
    		entry_header entry;
            memcpy(&entry, buf+pos, sizeof(entry));
    
    		int nameStart = pos + entry.name_offset_;
            int nlen = 0;
            {
                for(int n = nameStart; n < len; n++){
                    unsigned char tmp = buf[n];
                    if(tmp == 0x00){
                        break;
                    }
                    nlen++;
                }
            }
            std::string name((char*)(buf+nameStart), nlen);
            if(is_generation(name))fprintf(stderr, "name:%s, ", name.c_str()); 
    		int dataStart = pos + entry.data_offset_;
    		if(entry.vector_length_ == 0){
    			if(entry.data_type_ != 'J'){// tLong
    				fprintf(stderr, "\tUnexpected monitor type: %x\n", entry.data_type_);
                    return;
    			}
                int64_t value = 0;
                memcpy(&value, buf+dataStart, sizeof(value));

                if(is_generation(name))fprintf(stderr, "\tint_value:%ld\n", value); 
    		} 
            else{
    			if(entry.data_type_ != 'B' || //tByte || 
                    entry.data_units_ != 5 || //uString || 
                    (entry.data_var_ != 1 //vConstant 
                        && entry.data_var_ != 3 )){//vVariable)){
    				fprintf(stderr, 
                        "\tUnexpected vector monitor: data_type_:%x,data_units_:%x,data_var_:%x\n", 
                        entry.data_type_, entry.data_units_, entry.data_var_);
                    return;
    			}
                std::string value((char*)(buf+dataStart), entry.vector_length_); 
                if(is_generation(name))fprintf(stderr, "\tstr_value:%s\n", value.c_str()); 
    		}
    
    		pos += entry.entry_length_;
    	}
        delete []buf;
    }
};



#endif




