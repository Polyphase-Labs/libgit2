--- @meta

---@class ListViewWidget : Widget
ListViewWidget = {}

function ListViewWidget:SetItemTemplate() end

---@return Asset
function ListViewWidget:GetItemTemplate() end

function ListViewWidget:SetData() end

function ListViewWidget:AddItem() end

---@param index integer
function ListViewWidget:RemoveItem(index) end

---@param index integer
function ListViewWidget:UpdateItem(index) end

function ListViewWidget:Clear() end

---@return integer
function ListViewWidget:GetItemCount() end

---@param index integer
---@return table
function ListViewWidget:GetItemData(index) end

---@param spacing number
function ListViewWidget:SetSpacing(spacing) end

---@return number
function ListViewWidget:GetSpacing() end

function ListViewWidget:SetOrientation() end

---@return string
function ListViewWidget:GetOrientation() end

---@param width number
function ListViewWidget:SetItemWidth(width) end

---@return number
function ListViewWidget:GetItemWidth() end

---@param height number
function ListViewWidget:SetItemHeight(height) end

---@return number
function ListViewWidget:GetItemHeight() end

---@param index integer
function ListViewWidget:SetSelectedIndex(index) end

---@return integer
function ListViewWidget:GetSelectedIndex() end

---@return table
function ListViewWidget:GetSelectedData() end

function ListViewWidget:ClearSelection() end

---@return nil
function ListViewWidget:GetScrollContainer() end

---@return nil
function ListViewWidget:GetArrayWidget() end

---@param index integer
---@return nil
function ListViewWidget:GetItem(index) end

---@param index integer
function ListViewWidget:ScrollToItem(index) end
