--Fire Punch
sgs.ai_view_as.firepunch = function(card, player, card_place)
	local suit = card:getSuitString()
	local number = card:getNumberString()
	local card_id = card:getEffectiveId()
	if card:isRed() then
		return ("fire_slash:blackfeet[%s:%s]=%d"):format(suit, number, card_id)
	end
end

--Anti War
sgs.ai_skill_cardask["antiwar-invoke"] = function(self, data)
	local cards = sgs.QList2Table(self.player:getCards("he"))
	if #cards == 0 then return end
	self:sortByUseValue(cards, true)

	local damage = data:toDamage()

	local invoke = false
	if self:isFriend(damage.from) and damage.from:hasSkill(sgs.lose_equip_skill) then
		return "$"..cards[1]:getEffectiveId()
	elseif self:isEnemy(damage.from) then
		return "$"..cards[1]:getEffectiveId()
	end

	return "."
end
