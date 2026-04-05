--- @meta

---@class PlayerInputModule
PlayerInput = {}

---@param playerIndex? integer
---@return boolean
function PlayerInput.IsActive(playerIndex) end

---@param playerIndex? integer
---@return boolean
function PlayerInput.WasJustActivated(playerIndex) end

---@param playerIndex? integer
---@return boolean
function PlayerInput.WasJustDeactivated(playerIndex) end

---@param playerIndex? integer
---@return number
function PlayerInput.GetValue(playerIndex) end

function PlayerInput.RegisterAction() end

function PlayerInput.UnregisterAction() end

---@return integer
function PlayerInput.GetPlayersConnected() end

function PlayerInput.SetEnabled() end

---@return boolean
function PlayerInput.IsEnabled() end

---@param arg1 Asset
function PlayerInput.LoadActions(arg1) end
