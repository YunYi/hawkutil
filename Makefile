#---------------------------------------------
#添加规则
include Rules.mk

#设置SDK输出目录
HAWK_SDK = ../HawkSDK

#---------------------------------------------
#~~~~~~涉及目录设置
HAWK_DIRS   = HawkUtil HawkLog HawkGeometry HawkProfiler HawkGateway HawkRedis
TOOL_DIRS   = Tools/ProtocolGen Tools/ProcMonitor Tools/ProfilerMonitor Tools/LogServer Tools/DomainSvr Tools/GateServer Samples/TestCase Samples/EchoServer Samples/EchoClient

all: $(HAWK_DIRS) $(TOOL_DIRS)

$(HAWK_DIRS): force
	$(MAKE) -C $@

$(TOOL_DIRS): force
	$(MAKE) -C $@

install: force	
	-mkdir -p $(HAWK_SDK)
	-mkdir -p $(OUT_DIR)/sdk
	-mkdir -p $(OUT_DIR)/sdk/lib
	-mkdir -p $(OUT_DIR)/sdk/bin	
	-mkdir -p $(OUT_DIR)/sdk/include
	-cp -rf HawkUtil/*.h                $(OUT_DIR)/sdk/include
	-cp -rf HawkLog/*.h                 $(OUT_DIR)/sdk/include
	-cp -rf HawkGeometry/*.h            $(OUT_DIR)/sdk/include
	-cp -rf HawkProfiler/*.h            $(OUT_DIR)/sdk/include
	-cp -rf HawkGateway/*.h             $(OUT_DIR)/sdk/include
	-cp -rf HawkRedis/*.h               $(OUT_DIR)/sdk/include
	-cp -rf $(BIN_DIR)/libhawk*.a       $(OUT_DIR)/sdk/lib
	-cp -rf $(BIN_DIR)/protocolgen      $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/procmonitor      $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/echoserver       $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/echoclient       $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/testcase         $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/logserver        $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/domainsvr        $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/gateserver       $(OUT_DIR)/sdk/bin
	-cp -rf $(BIN_DIR)/profilermonitor  $(OUT_DIR)/sdk/bin
	-cp -rf $(OUT_DIR)/sdk/include      $(HAWK_SDK)
	-cp -rf $(OUT_DIR)/sdk/lib          $(HAWK_SDK)
	-cp -rf $(OUT_DIR)/sdk/bin          $(HAWK_SDK)

clean: force
	$(RM) $(OUT_DIR)
	-($(foreach dir,$(HAWK_DIRS),$(MAKE) -C $(dir) clean;))
	-($(foreach dir,$(TOOL_DIRS),$(MAKE) -C $(dir) clean;))

force:
