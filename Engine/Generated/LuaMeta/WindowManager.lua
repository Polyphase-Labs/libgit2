--- @meta

---@class WindowManagerModule
WindowManager = {}

---@param id string
---@return Node
function WindowManager.FindWindow(id) end

---@param id string
---@return boolean
function WindowManager.HasWindow(id) end

---@param id string
function WindowManager.ShowWindow(id) end

---@param id string
function WindowManager.HideWindow(id) end

---@param id string
function WindowManager.CloseWindow(id) end

---@param id string
function WindowManager.BringToFront(id) end
