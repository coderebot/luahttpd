
function debug(...)
    if DEBUG then
        print(...)
    end
end

------------------------------------------------------------------------------
-- Expand.lua
------------------------------------------------------------------------------

local strfind = string.find
local strsub  = string.sub
local push    = table.insert
local pop     = table.remove
local concat  = table.concat

local statements = {}
for w in string.gfind('do if for while repeat', '%a+') do
  statements[w] = true
end

local function expand(str, ...)
  assert(type(str)=='string', 'expecting string')
  local searchlist = arg
  local estring,evar

  function estring(str)
    local b,e,i
    b,i = strfind(str, '%$.')
    if not b then return str end

    local R, pos = {}, 1
    repeat
      b,e = strfind(str, '^%b{}', i)
      if b then
        push(R, strsub(str, pos, b-2))
        push(R, evar(strsub(str, b+1, e-1)))
        i = e+1
        pos = i
      else
        b,e = strfind(str, '^%b()', i)
        if b then
          push(R, strsub(str, pos, b-2))
          push(R, evar(strsub(str, b+1, e-1)))
          i = e+1
          pos = i
        elseif strfind(str, '^%a', i) then
          push(R, strsub(str, pos, i-2))
          push(R, evar(strsub(str, i, i)))
          i = i+1
          pos = i
        elseif strfind(str, '^%$', i) then
          push(R, strsub(str, pos, i))
          i = i+1
          pos = i
        end
      end
      b,i = strfind(str, '%$.', i)
    until not b

    push(R, strsub(str, pos))
    return concat(R)
  end

  local function search(index)
    for _,symt in ipairs(searchlist) do
      local ts = type(symt)
      local value
      if     ts == 'function' then value = symt(index)
      elseif ts == 'table'
          or ts == 'userdata' then value = symt[index]
          if type(value)=='function' then value = value(symt) end
      else error'search item must be a function, table or userdata' end
      if value ~= nil then return value end
    end
    error('unknown variable: '.. index)
  end

  local function elist(var, v, str, sep)
    local tab = search(v)
    if tab then
      assert(type(tab)=='table', 'expecting table from: '.. var)
      local R = {}
      push(searchlist, 1, tab)
      push(searchlist, 1, false)
      for _,elem in ipairs(tab) do
        searchlist[1] = elem
        push(R, estring(str))
      end
      pop(searchlist, 1)
      pop(searchlist, 1)
      return concat(R, sep)
    else
      return ''
    end
  end

  local function get(tab,index)
    for _,symt in ipairs(searchlist) do
      local ts = type(symt)
      local value
      if     ts == 'function' then value = symt(index)
      elseif ts == 'table'
          or ts == 'userdata' then value = symt[index]
      else error'search item must be a function, table or userdata' end
      if value ~= nil then
        tab[index] = value  -- caches value and prevents changing elements
        return value
      end
    end
  end

  function evar(var)
    if strfind(var, '^[_%a][_%w]*$') then -- ${vn}
      return estring(tostring(search(var)))
    end
    local b,e,cmd = strfind(var, '^(%a+)%s.')
    if cmd == 'foreach' then -- ${foreach vn xxx} or ${foreach vn/sep/xxx}
      local vn,s
      b,e,vn,s = strfind(var, '^([_%a][_%w]*)([%s%p]).', e)
      if vn then
        if strfind(s, '%s') then
          return elist(var, vn, strsub(var, e), '')
        end
        b = strfind(var, s, e, true)
        if b then
          return elist(var, vn, strsub(var, b+1), strsub(var,e,b-1))
        end
      end
      error('syntax error in: '.. var, 2)
    elseif cmd == 'when' then -- $(when vn xxx)
      local vn
      b,e,vn = strfind(var, '^([_%a][_%w]*)%s.', e)
      if vn then
        local t = search(vn)
        if not t then
          return ''
        end
        local s = strsub(var,e)
        if type(t)=='table' or type(t)=='userdata' then
          push(searchlist, 1, t)
          s = estring(s)
          pop(searchlist, 1)
          return s
        else
          return estring(s)
        end
      end
      error('syntax error in: '.. var, 2)
    else
      if statements[cmd] then -- do if for while repeat
        var = 'local OUT="" '.. var ..' return OUT'
      else  -- expression
        var = 'return '.. var
      end
      local f = assert(loadstring(var))
      local t = searchlist[1]
      assert(type(t)=='table' or type(t)=='userdata', 'expecting table')
      setfenv(f, setmetatable({}, {__index=get, __newindex=t}))
      return estring(tostring(f()))
    end
  end

  return estring(str)
end
---------------------------------------------------------------------------------------------

--------------------------------------------------------------------------------------------
-- proc http

if not read_line then
    read_line = function() return "\n" end
end
if not read_all then
    read_all = function() return "a=b&c=d" end
end

if not write_out then
    write_out = function (arg)
       print(arg)
    end

    METHOD = "GET"
    FILE = "htdocs/test2.lt"
    GET = {}
end

function dump_header ()
    if DEBUG then
        print ("METHOD=" .. tostring(METHOD))
        print ("FILE=" .. tostring(FILE))
        print ("GET=" .. tostring(GET))
    end
end


dump_header()

__ECHO_STR=""
__RESPONE_CODE="HTTP/1.0 200 OK"
__HEADER={}
__CONTENT_LEN = 0

-- read headers
function read_request_headers()
    --read_line
    repeat
        line = read_line()
        debug("header:",line)
        if (line == "\n") then break end
        m = string.gmatch(line, "(%w+)%s*:%s*([^\r\n]+)\n")
        if m then
            k,v = m()
            if k and v then
                __HEADER[k] = v
                if string.lower(k) == "content-length" then
                    __CONTENT_LEN = tonumber(v)
                end
            end
        end
    until line == "\n"
end

debug("read headers...")
read_request_headers()
debug("end read headers")

if METHOD and string.upper(METHOD) == 'POST' then
    content = read_all()
    debug("get post:"..tostring(content))

    POST= {}
    for k,v in string.gmatch(content, "(%w+)=([^&]+)") do
        POST[k] = v
    end
end


function respone_code(code, str)
   __RESPONE_CODE=string.format("HTTP/1.0 %d %s", code, str) 
end

function header(name, value)
    -- create the responed header
    if string.lower(name) == "content-length" then
        return
    end
    __HEADER[name] = value 
end

function echo (...)
    for i,v in ipairs(arg) do
        debug("echo:", v)
        __ECHO_STR = __ECHO_STR .. tostring(v)
    end
end

debug("file:"..tostring(FILE))
if string.sub(FILE,-3) == ".lt" then
    if get_htdocs then
        templates = get_htdocs(FILE)
    end
    
    if not templates or #templates == 0 then
        templates = ""
        for line in io.lines(FILE) do
            templates = templates .. line .. '\n'
        end
    end
    debug(templates)
    echo (expand(templates, _G, os.getenv))
else
    if get_htdocs then
        str = get_htdocs(FILE)
        if str then
            loadstring(str, FILE)()
        else
            dofile(FILE)
        end
    else
        dofile(FILE)
    end
end


-- write reponed
debug("begin out data")
write_out(__RESPONE_CODE .. "\r\n")
for k,v in pairs(__HEADER) do
    write_out(string.format("%s:%s\r\n", k,v))
end
write_out("Content-Type: text/html\r\n");
write_out(string.format("Content-Length: %d\r\n", #__ECHO_STR))
write_out("\r\n");

write_out(__ECHO_STR)

