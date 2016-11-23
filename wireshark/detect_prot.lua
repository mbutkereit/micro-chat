
scribe_proto = Proto("scribe","Scribe Protocol")


function getFlags (flags)
	local flag_name = ""
	
	if flags:uint() == 0 then
          flag_name= flag_name .. "NO_FLAG" 
        end
	if flags:bitfield(7,1) == 1 then
          flag_name= flag_name .. "SYN " 
        end
	if flags:bitfield(6,1) == 1 then
           flag_name= flag_name .. "ACK " 
        end
	if flags:bitfield(5,1) == 1 then
     	   flag_name= flag_name .. "FIN " 
        end
	if flags:bitfield(4,1)  == 1 then
     	   flag_name= flag_name .. "DUP " 
	end
	if flags:bitfield(3,1)  == 1 then
     	   flag_name= flag_name .. "GET " 
        end

	return flag_name;
end

function getTypeName (type_id)
       local type_name = type_id
       if type_name == 1 then
          type_name = "LOG_IN_OUT"
       elseif type_name == 2 then
          type_name = "CONTROLL_INFO"
       else
     	  type_name = "MESSAGE"
       end
      return type_name
end

function scribe_proto.dissector(buffer,pinfo,tree)
    pinfo.cols.protocol = "SCRIBE"
    local type = buffer(0,1):uint();
    local subtree = tree:add(scribe_proto,buffer(),"Scribe Protocol Data")
    local subtree_main = subtree:add(buffer(0,3),"Common Header")
    subtree_main:add(buffer(0,1),"Type: " .. getTypeName(buffer(0,1):uint()))
    subtree_main:add(buffer(1,1),"Flags: " .. getFlags(buffer(1,1)))
    subtree_main:add(buffer(2,1),"Version: " .. buffer(2,1):uint())

    if type == 1 then
       subtree_main:add(buffer(3,1),"Length: " .. buffer(3,1):bitfield(3,5))
       subtree_main:add(buffer(3,1),"Optional: " .. buffer(3,1):bitfield(0,3))
    else
      subtree_main:add(buffer(3,1),"Length: " .. buffer(3,1):uint())
    end
    local subtree_payload; 
   
    local length = buffer(3,1):uint();
    if length > 0 then 
     if type == 1 then
       subtree_payload = subtree:add(buffer(4,-1),"Payload")
       subtree_payload:add(buffer(4,-1),"Username: " .. buffer(3,-1):string())
     elseif type == 2 then
       subtree_payload = subtree:add(buffer(4,-1),"Payload")
       local size = 20;
       local elements = buffer(3,1):uint() - 1;
       for i=0,elements do
         local subtree_table_row = subtree_payload:add(buffer(4+(size*i),20),"Element:" .. i)
         subtree_table_row:add(buffer(4+(size*i),16),"Username:" .. buffer(4+(size*i),16):string())
         subtree_table_row:add(buffer(21+(size*i),2),"Hops:" .. buffer(21+(size*i),2):uint())         
        end
     elseif type == 3 then
       subtree_payload = subtree:add(buffer(4,-1),"Payload")
       subtree_payload:add(buffer(4,16),"Source Username: " .. buffer(4,16):string())
       subtree_payload:add(buffer(20,16),"Destination Username: " .. buffer(20,16):string())
       subtree_payload:add(buffer(36,-1),"Message: " .. buffer(36,-1):string())
     else
  	error("Invalid Header")
     end
    end
end

tcp_table = DissectorTable.get("tcp.port")

tcp_table:add(9012,scribe_proto)
