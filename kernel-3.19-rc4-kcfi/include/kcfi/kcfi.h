#ifndef KCFI_TAG

#ifdef CONFIG_KCFI

#define TAG_STR(tag) TAG_STR_HELPER(tag)
#define TAG_STR_HELPER(tag) #tag

#ifndef CONFIG_KCFI_COARSE

#define KCFI_TAG(tag) "nopl " tag " \n"

#else

#define KCFI_TAG(tag) "nopl 0x1337beef \n"

#endif /* CONFIG_KCFI_COARSE */

#else
#define KCFI_TAG(tag)
#define TAG_STR(tag)
#define TAG_STR_HELPER(tag)

#endif

#endif
