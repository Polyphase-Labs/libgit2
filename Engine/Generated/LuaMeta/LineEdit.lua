--- @meta

---@class LineEdit : Widget
LineEdit = {}

---@param title string
function LineEdit:SetTitle(title) end

---@return string
function LineEdit:GetTitle() end

---@param width number
function LineEdit:SetTitleWidth(width) end

---@return number
function LineEdit:GetTitleWidth() end

---@param text string
function LineEdit:SetText(text) end

---@return string
function LineEdit:GetText() end

---@param placeholder string
function LineEdit:SetPlaceholder(placeholder) end

---@return string
function LineEdit:GetPlaceholder() end

---@param pos integer
function LineEdit:SetCaretPosition(pos) end

---@return integer
function LineEdit:GetCaretPosition() end

function LineEdit:SelectAll() end

function LineEdit:ClearSelection() end

---@return boolean
function LineEdit:HasSelection() end

---@return string
function LineEdit:GetSelectedText() end

function LineEdit:DeleteSelection() end

---@param start integer
---@param end integer
function LineEdit:Select(start, end) end

---@return integer
function LineEdit:GetSelectionStart() end

---@return integer
function LineEdit:GetSelectionEnd() end

---@param focused boolean
function LineEdit:SetFocused(focused) end

---@return boolean
function LineEdit:IsFocused() end

---@param enabled boolean
function LineEdit:SetPasswordMode(enabled) end

---@return boolean
function LineEdit:IsPasswordMode() end

---@param maxLen integer
function LineEdit:SetMaxLength(maxLen) end

---@return integer
function LineEdit:GetMaxLength() end

---@param editable boolean
function LineEdit:SetEditable(editable) end

---@return boolean
function LineEdit:IsEditable() end

---@return Node
function LineEdit:GetTitleWidget() end

---@return Node
function LineEdit:GetInputField() end
