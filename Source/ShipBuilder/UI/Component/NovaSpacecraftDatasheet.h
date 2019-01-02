// Spaceship Builder - Gwennaël Arbona

#pragma once

#include "UI/NovaUI.h"
#include "UI/Widget/NovaTable.h"

#define LOCTEXT_NAMESPACE "SNovaSpacecraftDatasheet"

/** Spacecraft datasheet class */
class SNovaSpacecraftDatasheet : public SNovaTable
{
	SLATE_BEGIN_ARGS(SNovaSpacecraftDatasheet) : _TargetSpacecraft(), _ComparisonSpacecraft(nullptr)
	{}

	SLATE_ARGUMENT(FText, Title)
	SLATE_ARGUMENT(FNovaSpacecraft, TargetSpacecraft)
	SLATE_ARGUMENT(const FNovaSpacecraft*, ComparisonSpacecraft)

	SLATE_END_ARGS()

public:
	void Construct(const FArguments& InArgs)
	{
		// Fetch the required data
		const FNovaSpacecraft&                  Target                      = InArgs._TargetSpacecraft;
		const FNovaSpacecraft*                  Comparison                  = InArgs._ComparisonSpacecraft;
		const FNovaSpacecraftPropulsionMetrics& TargetPropulsionMetrics     = Target.GetPropulsionMetrics();
		const FNovaSpacecraftPropulsionMetrics* ComparisonPropulsionMetrics = Comparison ? &Comparison->GetPropulsionMetrics() : nullptr;

		SNovaTable::Construct(SNovaTable::FArguments().Title(InArgs._Title).Width(500));

		// Units
		FText Tonnes            = FText::FromString("T");
		FText MetersPerSeconds  = FText::FromString("m/s");
		FText MetersPerSeconds2 = FText::FromString("m/s-2");
		FText TonnesPerSecond   = FText::FromString("T/s");
		FText Seconds           = FText::FromString("s");
		FText KiloNewtons       = FText::FromString("kN");

		// Build the mass table
		AddHeader(LOCTEXT("Overview", "<img src=\"/Text/Module\"/> Overview"));
		AddEntry(LOCTEXT("Name", "Name"), Target.GetName(), Comparison ? Comparison->GetName() : FText());
		AddEntry(LOCTEXT("Classification", "Classification"), Target.GetClassification(),
			Comparison ? Comparison->GetClassification() : FText());
		AddEntry(LOCTEXT("Compartments", "Compartments"), Target.Compartments.Num(), Comparison ? Comparison->Compartments.Num() : -1);

		// Build the mass table
		AddHeader(LOCTEXT("MassMetrics", "<img src=\"/Text/Mass\"/> Mass metrics"));
		AddEntry(LOCTEXT("DryMass", "Dry mass"), TargetPropulsionMetrics.DryMass,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->DryMass : -1, Tonnes);
		AddEntry(LOCTEXT("PropellantMassCapacity", "Propellant capacity"), TargetPropulsionMetrics.PropellantMassCapacity,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->PropellantMassCapacity : -1, Tonnes);
		AddEntry(LOCTEXT("CargoMassCapacity", "Cargo capacity"), TargetPropulsionMetrics.CargoMassCapacity,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->CargoMassCapacity : -1, Tonnes);
		AddEntry(LOCTEXT("MaximumMass", "Full-load mass"), TargetPropulsionMetrics.MaximumMass,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumMass : -1, Tonnes);

		// Build the propulsion table
		AddHeader(LOCTEXT("PropulsionMetrics", "<img src=\"/Text/Thrust\"/> Propulsion metrics"));
		AddEntry(LOCTEXT("SpecificImpulse", "Specific impulse"), TargetPropulsionMetrics.SpecificImpulse,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->SpecificImpulse : -1, Seconds);
		AddEntry(LOCTEXT("Thrust", "Engine thrust"), TargetPropulsionMetrics.EngineThrust,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->EngineThrust : -1, KiloNewtons);
		AddEntry(LOCTEXT("LinearAcceleration", "Attitude control thrust"), TargetPropulsionMetrics.ThrusterThrust,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->ThrusterThrust : -1, KiloNewtons);
		AddEntry(LOCTEXT("MaximumDeltaV", "Full-load delta-v"), TargetPropulsionMetrics.MaximumDeltaV,
			ComparisonPropulsionMetrics ? ComparisonPropulsionMetrics->MaximumDeltaV : -1, MetersPerSeconds);
	}
};

#undef LOCTEXT_NAMESPACE
