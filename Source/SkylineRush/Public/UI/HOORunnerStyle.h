#pragma once
#include "CoreMinimal.h"
#include "Styling/SlateBrush.h"
namespace HOOAero
{
 inline const FLinearColor Panel(.012f,.028f,.046f,.94f), Surface(.025f,.055f,.078f,.96f),
 Text(.91f,.95f,.97f), Muted(.49f,.63f,.70f), Cyan(.025f,.74f,.83f),
 Coral(.98f,.22f,.12f), Gold(.96f,.68f,.21f), Fever(.96f,.16f,.46f), Ink(.006f,.020f,.030f);
 inline FSlateBrush Brush(FLinearColor C,float Radius=10)
 {
  FSlateBrush B;B.DrawAs=ESlateBrushDrawType::RoundedBox;B.TintColor=FSlateColor(C);
  B.OutlineSettings.CornerRadii=FVector4(Radius,Radius,Radius,Radius);
  B.OutlineSettings.RoundingType=ESlateBrushRoundingType::FixedRadius;
  B.OutlineSettings.Color=FSlateColor(FLinearColor(.08f,.29f,.34f,.7f));B.OutlineSettings.Width=1;
  return B;
 }
}