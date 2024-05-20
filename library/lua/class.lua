-- A trivial reloadable class system

local _ENV = mkmodule('class')

-- Metatable template for a class
---@class class.class_obj
---@overload fun(init_table: table): self
class_obj = class_obj or {}

-- Methods shared by all classes
---@class class.common_methods
common_methods = common_methods or {}

-- Forbidden names for class fields and methods.
---@class class.reserved_names
reserved_names = { super = true, ATTRS = true }

-- Attribute table metatable
---@class class.attrs_meta: table
---@overload fun(attributes: table)
attrs_meta = attrs_meta or {}

---@class dfhack.class: class.common_methods, class.class_obj
---@field super any
---@field ATTRS class.attrs_meta|fun(attributes: class.attrs_meta)

-- Create or updates a class; a class has metamethods and thus own metatable.
--
-- When defining a new class type with annotations, follow this pattern:
--
--    ---@class moduleName.ClassName: dfhack.class, moduleName.ParentClass?
--    ---@field super moduleName.ParentClass
--    ---@field ATTRS moduleName.ClassNameAttrs|fun(attributes: moduleName.ClassNameAttrs)
--    ---@overload fun(init_table: moduleName.ClassNameInitTable): self
--    ClassName = defclass(ClassName, ParentClass)
---@generic T: dfhack.class
---@param class? T
---@param parent? table
---@return T
function defclass(class,parent)
    class = class or {}

    local meta = getmetatable(class)
    if not meta then
        meta = {}
        setmetatable(class, meta)
    end

    for k,v in pairs(class_obj) do meta[k] = v end

    meta.__index = parent or common_methods

    local attrs = rawget(class, 'ATTRS') or {}
    setmetatable(attrs, attrs_meta)

    rawset(class, 'super', parent)
    rawset(class, 'ATTRS', attrs)
    rawset(class, '__index', rawget(class, '__index') or class)

    return class
end

-- An instance uses the class as metatable
---@generic T: dfhack.class
---@param class T
---@param table? table
---@return T
function mkinstance(class,table)
    table = table or {}
    setmetatable(table, class)
    return table
end

-- Patch the stubs in the global environment
dfhack.BASE_G.defclass = _ENV.defclass
dfhack.BASE_G.mkinstance = _ENV.mkinstance

-- Just verify the name, and then set.
function class_obj:__newindex(name,val)
    if reserved_names[name] or common_methods[name] then
        error('Method name '..name..' is reserved.')
    end
    rawset(self, name, val)
end

function attrs_meta:__call(attrs)
    for k,v in pairs(attrs) do
        self[k] = v
    end
end

local function apply_attrs(obj, attrs, init_table)
    for k,v in pairs(attrs) do
        local init_v = init_table[k]
        if init_v ~= nil then
            obj[k] = init_v
        elseif v == DEFAULT_NIL then
            obj[k] = nil
        else
            obj[k] = v
        end
    end
end

local function invoke_before_rec(self, class, method, ...)
    local meta = getmetatable(class)
    if meta then
        local fun = rawget(class, method)
        if fun then
            fun(self, ...)
        end

        invoke_before_rec(self, meta.__index, method, ...)
    end
end

local function invoke_after_rec(self, class, method, ...)
    local meta = getmetatable(class)
    if meta then
        invoke_after_rec(self, meta.__index, method, ...)

        local fun = rawget(class, method)
        if fun then
            fun(self, ...)
        end
    end
end

local function init_attrs_rec(obj, class, init_table)
    local meta = getmetatable(class)
    if meta then
        init_attrs_rec(obj, meta.__index, init_table)
        apply_attrs(obj, rawget(class, 'ATTRS'), init_table)
    end
end

-- Call metamethod constructs the object
function class_obj:__call(init_table)
    -- The table is assumed to be a scratch temporary.
    -- If it is not, copy it yourself before calling.
    init_table = init_table or {}

    local obj = mkinstance(self)

    -- This initialization sequence is broadly based on how the
    -- Common Lisp initialize-instance generic function works.

    -- preinit screens input arguments in subclass to superclass order
    invoke_before_rec(obj, self, 'preinit', init_table)
    -- initialize the instance table from init table
    init_attrs_rec(obj, self, init_table)
    -- init in superclass -> subclass
    invoke_after_rec(obj, self, 'init', init_table)
    -- postinit in superclass -> subclass
    invoke_after_rec(obj, self, 'postinit', init_table)

    return obj
end

-- Common methods for all instances:

---@param method string
---@param ... unknown
---@return unknown
function common_methods:callback(method, ...)
    return dfhack.curry(self[method], self, ...)
end

---@param field string
---@return function
function common_methods:cb_getfield(field)
    return function() return self[field] end
end

---@param field string
---@return function
function common_methods:cb_setfield(field)
    return function(val) self[field] = val end
end

---@param data table
function common_methods:assign(data)
    for k,v in pairs(data) do
        self[k] = v
    end
end

---@param method function
---@param ... unknown
---@return unknown
function common_methods:invoke_before(method, ...)
    return invoke_before_rec(self, getmetatable(self), method, ...)
end

---@param method function
---@param ... unknown
---@return unknown
function common_methods:invoke_after(method, ...)
    return invoke_after_rec(self, getmetatable(self), method, ...)
end

return _ENV
