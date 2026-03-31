--- @meta

---@class ListViewItemWidget : Widget
ListViewItemWidget = {}

---@return integer
function ListViewItemWidget:GetIndex() end

---@return nil
function ListViewItemWidget:GetListView() end

---@return nil
function ListViewItemWidget:GetContentWidget() end

---@param selected boolean
function ListViewItemWidget:SetSelected(selected) end

---@return boolean
function ListViewItemWidget:IsSelected() end

---@return boolean
function ListViewItemWidget:IsHovered() end
