import { CommonModule } from '@angular/common';
import { NgModule } from '@angular/core';
import { ReactiveFormsModule } from '@angular/forms';
import { RouterModule, Routes } from '@angular/router';

import { NgbNavModule, NgbTooltipModule } from '@ng-bootstrap/ng-bootstrap';

import { ActionLabels, URLVerbs } from '~/app/shared/constants/app.constants';
import { SharedModule } from '~/app/shared/shared.module';
import { BlockModule } from '../block/block.module';
import { CephSharedModule } from '../shared/ceph-shared.module';
import { CrushRuleFormModalComponent } from './crush-rule-form-modal/crush-rule-form-modal.component';
import { ErasureCodeProfileFormModalComponent } from './erasure-code-profile-form/erasure-code-profile-form-modal.component';
import { PoolDetailsComponent } from './pool-details/pool-details.component';
import { PoolFormComponent } from './pool-form/pool-form.component';
import { PoolListComponent } from './pool-list/pool-list.component';
import { IconModule, IconService } from 'carbon-components-angular';
import HelpIcon from '@carbon/icons/es/help/16';
import UnlockedIcon from '@carbon/icons/es/unlocked/16';
import LockedIcon from '@carbon/icons/es/locked/16';

@NgModule({
  imports: [
    CephSharedModule,
    CommonModule,
    NgbNavModule,
    SharedModule,
    RouterModule,
    ReactiveFormsModule,
    NgbTooltipModule,
    BlockModule,
    IconModule
  ],
  exports: [PoolListComponent, PoolFormComponent],
  declarations: [
    PoolListComponent,
    PoolFormComponent,
    ErasureCodeProfileFormModalComponent,
    CrushRuleFormModalComponent,
    PoolDetailsComponent
  ]
})
export class PoolModule {
  constructor(private iconService: IconService) {
    this.iconService.registerAll([HelpIcon, UnlockedIcon, LockedIcon]);
  }
}

const routes: Routes = [
  { path: '', component: PoolListComponent },
  {
    path: URLVerbs.CREATE,
    component: PoolFormComponent,
    data: { breadcrumbs: ActionLabels.CREATE }
  },
  {
    path: `${URLVerbs.EDIT}/:name`,
    component: PoolFormComponent,
    data: { breadcrumbs: ActionLabels.EDIT }
  }
];

@NgModule({
  imports: [PoolModule, RouterModule.forChild(routes)]
})
export class RoutedPoolModule {}
