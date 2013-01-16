#���ӹ���
include ../Rules.mk

#��Ŀ¼����
TINYXML_DIR = source/tinyxml
ZLIB_DIR	= source/zlib
JSON_DIR    = source/json
SQLITE_DIR  = source/sqlite

#����ͷ�ļ�·��
INC_DIRS    += -I$(TINYXML_DIR) -I$(ZLIB_DIR) -I$(JSON_DIR) -I$(SQLITE_DIR) -Isource/nedmalloc -Ilnx_inc/libevent -Ilnx_inc/libzmq

#���ɿ��ļ�
LIB_HAWKUTIL = ../$(BIN_DIR)/libhawkutil.a

#������Ŀ¼
SUB_DIRS     = $(TINYXML_DIR) $(ZLIB_DIR) $(JSON_DIR) $(SQLITE_DIR)

#����������ļ�
DEP_LIBS     = lnx_lib/libevent.a lnx_lib/libzmq.a

#�궨���޸�
MACROS += -DUTIL_EXPORT

#�����ǩ
all: $(SUB_DIRS) $(OBJ_FILES) $(LIB_HAWKUTIL)

$(SUB_DIRS): force
	$(MAKE) -C $@

$(LIB_HAWKUTIL): force
	cd $(OBJ_DIR) ;$(foreach lib,$(DEP_LIBS),ar x ../../$(lib);)
	$(AR) $@ $(OBJ_DIR)/*.o $(TINYXML_DIR)/$(OBJ_DIR)/*.o $(ZLIB_DIR)/$(OBJ_DIR)/*.o $(JSON_DIR)/$(OBJ_DIR)/*.o $(SQLITE_DIR)/$(OBJ_DIR)/*.o

clean: force
	$(RM) $(OUT_DIR)
	-($(foreach dir,$(SUB_DIRS),$(MAKE) -C $(dir) clean;))

force: