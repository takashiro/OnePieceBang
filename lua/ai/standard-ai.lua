
--Rubber Pistol
sgs.ai_skill_invoke.rubberpistol = function(self, data)
	return true
end

--Frety Wind
sgs.ai_skill_invoke.fretywind = function(self, data)
	local choice = sgs.ai_skill_choice.fretywind
	if(choice ~= "nothing") then
		return true
	end
	return false	 
end

sgs.ai_skill_choice.fretywind = function(self, choices)
	self:sort(self.enemies, "defense")
	local n1 = self:getCardsNum("Slash")
	local slash = sgs.Card_Parse(("slash[%s:%s]"):format(sgs.Card_NoSuit, 0))
	for _, enemy in ipairs(self.enemies) do
		local n2 = self:getCardsNum("Slash", enemy)
		if n1 >= n2 then
			return "duel"
		elseif not self:slashProhibit(slash ,enemy) then
			return "slash"
		end
	end
	return "nothing"
end

sgs.ai_skill_playerchosen.fretywind = sgs.ai_skill_playerchosen.zero_card_as_slash
