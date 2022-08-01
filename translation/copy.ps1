# USER DEFINED
$outDir = "C:\Skyrim Mod Organizer\mods\Precision - Accurate Melee Collisions\Interface\Translations"

$strings = @('chinese', 'czech', 'english', 'french', 'german', 'italian', 'japanese', 'polish', 'russian', 'spanish')

ForEach ($string in $strings)
{
    Copy-Item "Precision_english.txt" -Destination "$outDir\Precision_$string.txt"
}