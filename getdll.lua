local dllpath = os.getenv('CPATH') .. '\\..\\dll64\\'
local exe = arg[1]

function table.find (t, x)
for k, v in pairs(t) do
if v==x then return k end
end end

local function dllList (fn, t)
t = t or {}
local dfp = io.popen('objdump -p ' .. fn .. ' | grep -i "DLL Name:"', 'r')
for l in dfp:lines() do
local dll = l:sub(12)
if table.find(t, dll) then goto continue end
local fr = io.open(dllpath..dll, 'rb')
if not fr then goto continue end
table.insert(t, dll)
fr:close()
::continue:: end
dfp:close()
return t
end

local dlls = dllList(exe)
do local i = 1 repeat
local dll = dlls[i]
dllList(dllpath..dll, dlls)
i = i+1
until i > #dlls end

for i, dll in ipairs(dlls) do
local fw = io.open(dll, 'rb')
if fw then 
fw:close() 
goto continue
end
local fr = io.open(dllpath..dll, 'rb')
if not fr then goto continue end
fw = io.open(dll, 'wb')
if not fw then goto continue end
fw:write(fr:read('a'))
fw:close()
fr:close()
print('Copied '..dll)
::continue:: end
