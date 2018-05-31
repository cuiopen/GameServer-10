


key = ""
function PrintTable(table , level)
  level = level or 1
  local indent = ""
  for i = 1, level do
    indent = indent.."  "
  end

  if key ~= "" then
    print(indent..key.." ".."=".." ".."{")
  else
    print(indent .. "{")
  end

  key = ""
  for k,v in pairs(table) do
     if type(v) == "table" then
        key = k
        PrintTable(v, level + 1)
     else
        local content = string.format("%s%s = %s", indent .. "  ",tostring(k), tostring(v))
      print(content)  
      end
  end
  print(indent .. "}")

end
--[[
MysqlWrapper.Connect("127.0.0.1", 3306, "test", "root","123456",function (err,context)
    if(err) then
        print(err)
        return
    else
        print("Connect success")
        MysqlWrapper.Query(context, "select * from test", function (err, ret)
		if err then 
			print(err)
			return;
		end
		if ret then
			PrintTable(ret)
			print("query success")
			return;
		else
			print("result is null")
		end
	end)
        MysqlWrapper.Close(context);
    end

end )]]
--[[
RedisWrapper.Connect("127.0.0.1",6379, function(err,context)
    if(err) then
        print(err)
        return
    else
        print("Connect success")
        RedisWrapper.Query(context, "hmset 001001 name \"Vetex\" age \"20\"", function (err, ret)
            if err then
                print(err)
                return;
            end
            if type(ret)=="table" then
                PrintTable(ret)
                print("query success")
                return;
            else
                if ret then
                    print(ret)
                else
                    print("result is null")
                end

            end
        end)
        RedisWrapper.Close(context);
    end

end)]]

local my_service = {
  -- msg {1: stype, 2 ctype, 3 utag, 4 body_table_or_str}
  on_session_recv_cmd = function(session, msg)
    
  end,

  on_session_disconnect = function(session)
  end
}

local ret = Service.RegisterService(100, my_service)
print(ret);