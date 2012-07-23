<?php

function gd_deposit_form($form, &$form_state) {
  $groupid = arg(1);
  $group = new GD_Group($groupid);

  // create memberlist
  $memberlist = array();
  foreach ($group->get_members() as $memberid => $details) {
    $memberlist[$memberid] = GDme()->format_name($details);
  }
  natcasesort($memberlist);

  // create event list
  // create from date, for now take 1 month back
  $from = date('Y-m-d', mktime(0, 0, 0, date("m") - 1, date("d"), date("Y")));
  // put in temp array for sorting
  $eventSummaryList = bs_array_sort($group->get_event_list($from), 'event_timestamp');
  $eventlist = array();
  foreach ($eventSummaryList as $eventid => $details) {
    $eventlist[$eventid] = format_date($details->event_timestamp, 'gd_expense_date') . ' - ' . $details->event_name;
  }

  // create type list
  $typelist = array();
  foreach (GDme()->get_expense_types(TRUE) as $id => $desc) {
    $typelist[$id] = $desc['description'];
  }
  natsort($typelist);

  // Expense by (dropdown)
  $form['gd_deposit_owner'] = array(
      '#type' => 'select',
      '#title' => t('Deposit owner'),
      '#description' => t('The person that will collect the deposits.'),
      '#options' => $memberlist,
      '#required' => TRUE,
      '#default_value' => GDme()->Details->user_id,
  );

  // Event (dropdown)
  $eventlist = count($eventlist) > 0 ? $eventlist : $eventlist = array(0 => t('No current events found'));
  $form['gd_deposit_event'] = array(
      '#type' => 'select',
      '#title' => t('Event'),
      '#description' => t('Event the group deposit belongs to.'),
      '#options' => $eventlist,
      '#required' => FALSE,
  );

  // Description
  $form['gd_deposit_desc'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#description' => t('Brief description of the group deposit.'),
      '#size' => 30,
      '#maxlength' => 60,
      '#required' => TRUE,
  );

  // Amount
  $form['gd_deposit_amount'] = array(
      '#type' => 'textfield',
      '#title' => t('Amount per person'),
      '#description' => t('How much each person puts in the group deposit.'),
      '#size' => 10,
      '#maxlength' => 6,
      '#required' => TRUE,
      '#element_validate' => array('is_formfield_below999with2digits'),
  );

  // Expense date
  $form['gd_deposit_date'] = array(
      '#type' => 'date_popup',
      '#title' => t('Date'),
      '#description' => t('Date when the deposit was made.'),
      '#required' => TRUE,
  );

  // Participants
  $form['gd_deposit_participants'] = array(
      '#title' => t('Participants'),
      '#description' => t('Who participated in the deposit.'),
      '#type' => 'checkboxes',
      '#options' => $memberlist,
      '#multicolumn' => array('width' => 3),
      '#required' => TRUE,
  );

  // Submit
  $form['gd_submit'] = array(
      '#type' => 'submit',
      '#value' => t('Submit'),
  );
  return $form;
}

function gd_deposit_form_submit($form, &$form_state) {
  $groupid = arg(1);
  $val = $form_state['values'];
  $eventid = empty($val['gd_deposit_event']) ? 0 : $val['gd_deposit_event'];
  if ($form['gd_deposit_date']['#type'] == 'date')
    $date = "{$val['gd_deposit_date']['year']}-{$val['gd_deposit_date']['month']}-{$val['gd_deposit_date']['day']}";
  else
    $date = $val['gd_deposit_date'];
  gd_connect_db();
  // first register deposit to get a deposit id
  $deposit_id = db_insert('deposits')->fields(array(
              'holder' => $val['gd_deposit_owner'],
              'description' => $val['gd_deposit_desc'],
          ))->execute();

  $values = array();
  // for each person, make an expense with only deposit holder ($userid) as recipient
  foreach ($val['gd_deposit_participants'] as $memberid) {
    if ($memberid > 0) {
      $expense_id = db_insert('expenses')->fields(array(
                  'type' => 0,
                  'user_id' => $memberid,
                  'group_id' => $groupid,
                  'description' => $val['gd_deposit_desc'],
                  'amount' => $val['gd_deposit_amount'],
                  'expense_date' => $date,
                  'event_id' => $eventid,
                  'currency' => '1',
                  'deposit_id' => $deposit_id
              ))->execute();
      $values[] = array('user_id' => $val['gd_deposit_owner'], 'expense_id' => $expense_id);
    }
  }
  $query = db_insert('users_expenses')->fields(array('user_id', 'expense_id'));
  foreach ($values as $record) {
    $query->values($record);
  }
  $query->execute();
  db_set_active();
  drupal_set_message(t('Deposit has been recorded'));
}

function is_formfield_below999with2digits($element, &$form_state) {
  if (empty($element['#value']))
    return;
  if (!(preg_match("/^\d{1,3}([\.,]\d{1,2})?$/D", $element['#value'])))
    form_error($element, t('The "!name" entered is not valid', array('!name' => t($element['#title']))));
  else {
    if (strpos($element['#value'], ',')) {
      // comma found, make it a dot
      form_set_value($element, str_replace(',', '.', $element['#value']), $form_state);
    }
  }
}