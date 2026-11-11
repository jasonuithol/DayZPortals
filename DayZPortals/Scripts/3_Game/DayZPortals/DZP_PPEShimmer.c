// Air shimmer near an assembled gate: a wobbling lens distortion centred on the grille's
// screen position, with a touch of chromatic aberration. Vanilla has no heat-haze particle
// (its fires don't refract), so this is done in post-processing. Driven every frame by
// DZP_GateShimmer (4_World).
class DZP_PPERequester_GateShimmer extends PPERequester_GameplayBase
{
	// strength 0..1; centre in screen space, -1..1 from the middle (as PPEDistort wants it)
	void SetShimmer(float distort, float chromAbb, float centreX, float centreY)
	{
		SetTargetValueFloat(PostProcessEffectType.Distort, PPEDistort.PARAM_DISTORT, false, distort, PPEDistort.L_0_HMP, PPOperators.ADD);
		SetTargetValueFloat(PostProcessEffectType.Distort, PPEDistort.PARAM_CENTERX, false, centreX, PPEDistort.L_1_HMP, PPOperators.ADD);
		SetTargetValueFloat(PostProcessEffectType.Distort, PPEDistort.PARAM_CENTERY, false, centreY, PPEDistort.L_2_HMP, PPOperators.ADD);
		SetTargetValueFloat(PostProcessEffectType.Distort, PPEDistort.PARAM_CHROM_ABB, true, chromAbb, PPEDistort.L_3_HMP, PPOperators.ADD);
	}
}

modded class PPERequesterRegistrations
{
	override protected void RegisterAdditionalRequesters()
	{
		super.RegisterAdditionalRequesters();
		PPERequesterBank.RegisterRequester(DZP_PPERequester_GateShimmer);
	}
}
