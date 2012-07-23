<?php

function gd_event_form($form, &$form_state) {
  $groupid = arg(1);
  $group = new GD_Group($groupid);

  // create memberlist
  $memberlist = array();
  foreach ($group->get_members() as $memberid => $details) {
    $memberlist[$memberid] = GDme()->format_name($details);
  }
  natcasesort($memberlist);

  // create type list
  $typelist = array();
  foreach (GDme()->get_expense_types(TRUE) as $id => $desc) {
    $typelist[$id] = $desc['description'];
  }
  natsort($typelist);

  // Name
  $form['gd_event_name'] = array(
      '#type' => 'textfield',
      '#title' => t('Name'),
      '#description' => t('Name of the event.'),
      '#size' => 30,
      '#maxlength' => 30,
      '#required' => TRUE,
  );

  // Description
  $form['gd_event_desc'] = array(
      '#type' => 'textfield',
      '#title' => t('Description'),
      '#description' => t('Brief description of the event.'),
      '#size' => 30,
      '#maxlength' => 60,
      '#required' => TRUE,
  );

  // Event date
  global $language;
  $date_popup_format = ($language->language == 'nl') ? 'd-m-Y' : 'm/d/Y';
  $form['gd_event_date'] = array(
      '#type' => 'date_popup',
      '#date_type' => DATE_ISO,
      '#date_format' => $date_popup_format,
      '#default_value' => 'YYYY-MM-DD 00:00:00',
      '#title' => t('Date'),
      '#description' => t('Date when the event starts.'),
      '#required' => TRUE,
  );

  // Type (dropdown)
  $form['gd_expense_type'] = array(
      '#type' => 'select',
      '#title' => t('Default payment type'),
      '#description' => t('The expected most used type of payment for this event.'),
      '#options' => $typelist,
      '#required' => FALSE,
  );

  // Participants
  $form['gd_event_participants'] = array(
      '#title' => t('Participants'),
      '#description' => t('The persons that will probably participate in this event.'),
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

function gd_event_form_submit($form, &$form_state) {
  $groupid = arg(1);
  $val = $form_state['values'];
  $eventid = empty($val['gd_event_id']) ? 0 : $val['gd_event_id'];
  if ($form['gd_expense_date']['#type'] == 'date')
    $date = "{$val['gd_expense_date']['year']}-{$val['gd_expense_date']['month']}-{$val['gd_expense_date']['day']}";
  else
    $date = $val['gd_expense_date'];
  gd_connect_db();
  $event_id = db_insert('events')->fields(array(
      'event_name' => $val['gd_event_name'],
      'event_description' => $val['gd_event_desc'],
      'group_id' => $groupid,
      'organizer_id' => GDme()->Details->user_id,
      'date' => $val['gd_event_date'],
      'expense_type_id' => $val['gd_expense_type'],
  ))->execute();
  
  $values = array();
  foreach ($val['gd_expense_participants'] as $memberid) {
    if ($memberid > 0)
      $values[] = array('user_id' => $memberid, 'event_id' => $event_id);
  }
  $query = db_insert('users_events')->fields(array('user_id', 'event_id'));
  foreach ($values as $record) {
    $query->values($record);
  }
  $query->execute();
  db_set_active();
  drupal_set_message(t('Event  has been recorded'));
}
