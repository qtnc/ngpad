function identity (...)
return ...
end

function isstring (x)
return type(x) == 'string'
end

function isnumber (x)
return type(x) == 'number'
end

function istable (x)
return type(x) == 'table'
end

function isfunction (x)
return type(x) == 'function'
end

function table.foreach (t, f, ...)
local itr = #t>0 and ipairs or pairs
for k, v in itr(t) do f(k, v, ...) end
end

function table.range (start, finish, incr)
local r = {}
incr = incr or (finish<start and -1 or 1)
for i = start, finish, incr do table.insert(r, i) end
return r
end

function table.map (t, keyMapper, valueMapper, ...)
local r = setmetatable({}, getmetatable(t))
if not valueMapper then
keyMapper, valueMapper = identity, keyMapper
end
for k, v in pairs(t) do
k = keyMapper(k, ...)
v = valueMapper(v, ...)
if k then rawset(r, k, v) end
end
return r
end

function table.filter (t, keyFilter, valueFilter, ...)
if not valueFilter then
keyFilter, valueFilter = nil, keyFilter
end
local r = setmetatable({}, getmetatable(t))
for k, v in pairs(t) do
if (not keyFilter or keyFilter(k, ...))
and (not valueFilter or valueFilter(v, ...))
then rawset(r, k, v) end
end
return r
end

function table.collect (...)
local r = {}
for k, v in ... do
if v==nil then table.insert(r, k)
else r[k]=v
end end
return r
end

function table.copy (t, r)
r = setmetatable(r or {}, getmetatable(t))
for k, v in pairs(t) do
if istable(v) then v = table.copy(v) end
rawset(r, k, v)
end
return r
end

function table.insertall (t, pos, s)
if not s then
s = pos
pos = #t+1
end
table.move(t, pos, #t, pos+#s, t)
return table.move(s, 1, #s, pos, t)
end

function table.slice (t, i, j)
return table.move(t, i or 1, j or #t, 1, {})
end

function table.splice (t, i, j, s)
s = s or {}
local r = table.move(t, i, j, 1, {})
table.move(t, j+1, #t+#s+#r, i+#s, t)
table.move(s, 1, #s, i, t)
return r
end

function table.find (t, v, start)
for i = start or 1, #t do
local x = t[i]
if x==v then return i, v end
end 
for i, x in pairs(t) do
if x==v then return i, v end
end 
end

function table.findif (t, f, i, ...)
for i = start or 1, #t do
local v = t[i]
local r = f(v, ...)
if r then return i, v, r end
end 
for i, v in pairs(t) do
local r = f(v, ...)
if r then return i, v, r end
end 
end

function table.join (t, sep, i, j, prefix, suffix) 
if isstring(i) or isstring(j) then
i, j, prefix, suffix = prefix, suffix, i, j
end
prefix, suffix = prefix or '', suffix or ''
return prefix .. table.concat(table.map(t, tostring), sep, i, j) .. suffix
end

function string.join (sep, t, i, j, prefix, suffix)
return table.join(t, sep, i, j, prefix, suffix)
end

local function fieldtostring (v, ...)
if istable(v) then
local mt = getmetatable(v)
return mt and mt.__tostring and mt.__tostring(t) or table.tostring(t, ...)
elseif isnumber(v) then return tostring(v)
else return string.format('%q', v) 
end end

function table.tostring (t, sep, prefix, suffix, kvsep, kprefix, ksuffix)
sep = sep or ', '
kvsep = kvsep or '='
prefix = prefix or '{'
suffix = suffix or '}'
kprefix = kprefix or '['
ksuffix = ksuffix or ']'
local r = {}
if #t>0 then
for k, v in ipairs(t) do
r[k] = fieldtostring(v, sep, prefix, suffix, kvsep, kprefix, ksuffix)
end
else -- #t==0
for k, v in pairs(t) do
table.insert(r, table.concat{ kprefix, fieldtostring(k), ksuffix, kvsep, fieldtostring(v, sep, prefix, suffix, kvsep, kprefix, ksuffix) })
end
end
return prefix .. table.concat(r, sep) .. suffix
end
