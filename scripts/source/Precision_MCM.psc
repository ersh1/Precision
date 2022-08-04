ScriptName Precision_MCM Extends MCM_ConfigBase

Bool Property bSweepAttackModeMaxTargets Auto
Bool Property bSweepAttackModeDiminishingReturns Auto

Event OnConfigClose() native

Event OnConfigOpen()
    bSweepAttackModeMaxTargets = GetModSettingInt("uSweepAttackMode:AttackCollisions") == 1
    bSweepAttackModeDiminishingReturns = GetModSettingInt("uSweepAttackMode:AttackCollisions") == 2
EndEvent

Event OnSettingChange(String a_ID)
    if (a_ID == "uSweepAttackMode:AttackCollisions")
        bSweepAttackModeMaxTargets = GetModSettingInt(a_ID) == 1
        bSweepAttackModeDiminishingReturns = GetModSettingInt(a_ID) == 2
        RefreshMenu()
    endif
EndEvent