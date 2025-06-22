void *memcpy(void *dest, const void *src, unsigned int len){
	const unsigned char *source = (const unsigned char *)src;
	unsigned char *destination = (unsigned char *)dest;

	for(unsigned int i = 0; i < len; i++){
		destination[i] = source[i];
	}
	return dest;
}

void *memset(void *dest, int value, unsigned int len) {
    unsigned char *ptr = (unsigned char *)dest;

    for (unsigned int i = 0; i < len; i++) {
        ptr[i] = (unsigned char)value;
    }

    return dest;
}
